#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_timer.h>

#include "motion.h"


uint32_t counter = 0;

uint32_t last_interrupt_time = 0;

static const char* TAG = "motion sensor";

static QueueHandle_t gpio_evt_queue = NULL;


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t current_time = esp_timer_get_time() / 1000; // Get time in milliseconds

    if ((current_time - last_interrupt_time) > DEBOUNCE_DELAY_MS) {
        last_interrupt_time = current_time;
        uint32_t gpio_num = (uint32_t) arg;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
        counter = 0;
    }
}

void timer_callback(void *arg) {
    // Increment counter every second
    counter++;
}

void sleep_task(void *pvParameter) {
    while (1) {
        // Check if counter reaches half hour (1800 seconds)
        if (counter >= 1800) {
            printf("Yo dawg...TODO\n");
            // TODO - This is where you would put the clock to sleep
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Check every second
    }
}

static void gpio_task(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Motion detected on GPIO %u", (unsigned int) io_num);
        }
    }
}

void motion_init(void) {

    /* Start motion sensor */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = true;
    // io_conf.pull_up_en = false;
    io_conf.pin_bit_mask = (1ULL<<GPIO_MOTION_INTR_PIN);
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // //start gpio task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_MOTION_INTR_PIN, gpio_isr_handler, (void*) GPIO_MOTION_INTR_PIN);

    // printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

    // Create and start timer
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "periodic_timer"
    };
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, TIMER_INTERVAL * 1000);  // Timer interval in microseconds

    // Create task to monitor counter
    xTaskCreate(&sleep_task, "sleep_task", 4096, NULL, 5, NULL);
}