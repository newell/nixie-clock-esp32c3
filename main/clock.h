#ifndef CLOCK_H
#define CLOCK_H

#define OE_PIN      GPIO_NUM_3  // Output Enable pin connected to the shift register (~OE)
#define DATA_PIN    GPIO_NUM_4  // Data pin connected to the shift register (SER)
#define LATCH_PIN   GPIO_NUM_5  // Latch pin connected to the shift register (RCLK)
#define CLOCK_PIN   GPIO_NUM_6  // Clock pin connected to the shift register (SRCLK)
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<DATA_PIN) | (1ULL<<LATCH_PIN) | (1ULL<<CLOCK_PIN))

void clock_init(void);

#endif /* CLOCK_H */
