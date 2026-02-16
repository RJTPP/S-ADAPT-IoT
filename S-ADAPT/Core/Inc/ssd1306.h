/**
 * ssd1306.h
 *
 *  Created on: Feb 13, 2026
 *      Author: Antigravity
 */

#ifndef SSD1306_H_
#define SSD1306_H_

#include "stm32l4xx_hal.h"
#include "fonts.h"

/* SSD1306 I2C Address */
#define SSD1306_I2C_ADDR        (0x3C << 1)

/* SSD1306 Settings */
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64

/* Color enumeration */
typedef enum {
    Black = 0x00, /*!< Black color, no pixel */
    White = 0x01  /*!< White color, pixel is set */
} SSD1306_COLOR;

/* Struct to store transformations */
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
} SSD1306_t;

/* Function Prototypes */
uint8_t SSD1306_Init(void);
void SSD1306_UpdateScreen(void);
void SSD1306_Fill(SSD1306_COLOR color);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
char SSD1306_WriteChar(char ch, sFONT font, SSD1306_COLOR color);
char SSD1306_WriteString(char* str, sFONT font, SSD1306_COLOR color);
void SSD1306_SetCursor(uint8_t x, uint8_t y);

#endif /* SSD1306_H_ */
