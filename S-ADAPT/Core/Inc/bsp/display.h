#ifndef DISPLAY_H
#define DISPLAY_H

#include "stm32l4xx_hal.h"

uint8_t display_init(void);
void display_show_boot(void);
void display_show_distance_cm(uint32_t distance_cm);

#endif /* DISPLAY_H */
