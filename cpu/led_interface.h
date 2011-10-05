#ifndef LED_INTERFACE_H
#define LED_INTERFACE_H

#include "../common.h"

uint32_t	CORE_red_led_read(void);
void		CORE_red_led_write(uint32_t val);
uint32_t	CORE_grn_led_read(void);
void		CORE_grn_led_write(uint32_t val);
uint32_t	CORE_blu_led_read(void);
void		CORE_blu_led_write(uint32_t val);

#endif // LED_INTERFACE_H
