#ifndef LEDS_H
#define LEDS_H

#include <led_strip.h>

extern led_strip_handle_t led_strip;

void party_lights(void);
void set_leds_to_config(led_strip_handle_t *led_strip);
void configure_leds(void);

#endif /* LEDS_H */
