#ifndef MOTION_H
#define MOTION_H

#define DEBOUNCE_DELAY_MS 1000  // 1 second, very sensitive GPIO
#define TIMER_INTERVAL 1000     // 1 second
#define GPIO_MOTION_INTR_PIN GPIO_NUM_10

void motion_init(void);

#endif /* MOTION_H */
