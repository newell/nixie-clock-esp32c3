#ifndef LEDS_H
#define LEDS_H

#define BLINK_GPIO CONFIG_BLINK_GPIO

void power_up_light_sequence(void);
void configure_leds(void);

#endif /* LEDS_H */
