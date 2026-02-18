/**
 * ssd1306.c
 *
 *  Created on: Feb 13, 2026
 *      Author: Antigravity
 */

#include "ssd1306.h"
#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Screen object
static SSD1306_t SSD1306;
static uint16_t SSD1306_ActiveAddress = SSD1306_I2C_ADDR;

static uint8_t SSD1306_WriteCommand(uint8_t command) {
    return HAL_I2C_Mem_Write(&hi2c1, SSD1306_ActiveAddress, 0x00, 1, &command, 1, 10);
}

uint8_t SSD1306_Init(void) {
    HAL_StatusTypeDef status;
    uint16_t candidate_addresses[] = { (0x3C << 1), (0x3D << 1) };
    uint8_t found = 0;

    HAL_Delay(100);

    for (uint32_t i = 0; i < (sizeof(candidate_addresses) / sizeof(candidate_addresses[0])); i++) {
        status = HAL_I2C_IsDeviceReady(&hi2c1, candidate_addresses[i], 2, 50);
        if (status == HAL_OK) {
            SSD1306_ActiveAddress = candidate_addresses[i];
            found = 1;
            break;
        }
    }

    if (!found) {
        SSD1306.Initialized = 0;
        return 0;
    }

    /* Init LCD */
    if (SSD1306_WriteCommand(0xAE) != HAL_OK) return 0; // display off
    if (SSD1306_WriteCommand(0x20) != HAL_OK) return 0; // Set Memory Addressing Mode
    if (SSD1306_WriteCommand(0x10) != HAL_OK) return 0; // Page Addressing Mode
    if (SSD1306_WriteCommand(0xB0) != HAL_OK) return 0; // Set Page Start Address
    if (SSD1306_WriteCommand(0xC8) != HAL_OK) return 0; // Set COM Output Scan Direction
    if (SSD1306_WriteCommand(0x00) != HAL_OK) return 0; // set low column address
    if (SSD1306_WriteCommand(0x10) != HAL_OK) return 0; // set high column address
    if (SSD1306_WriteCommand(0x40) != HAL_OK) return 0; // set start line address
    if (SSD1306_WriteCommand(0x81) != HAL_OK) return 0; // contrast control register
    if (SSD1306_WriteCommand(0xFF) != HAL_OK) return 0;
    if (SSD1306_WriteCommand(0xA1) != HAL_OK) return 0; // segment re-map 0 to 127
    if (SSD1306_WriteCommand(0xA6) != HAL_OK) return 0; // normal display
    if (SSD1306_WriteCommand(0xA8) != HAL_OK) return 0; // multiplex ratio
    if (SSD1306_WriteCommand(0x3F) != HAL_OK) return 0;
    if (SSD1306_WriteCommand(0xA4) != HAL_OK) return 0; // output follows RAM content
    if (SSD1306_WriteCommand(0xD3) != HAL_OK) return 0; // set display offset
    if (SSD1306_WriteCommand(0x00) != HAL_OK) return 0;
    if (SSD1306_WriteCommand(0xD5) != HAL_OK) return 0; // set display clock divide ratio
    if (SSD1306_WriteCommand(0xF0) != HAL_OK) return 0;
    if (SSD1306_WriteCommand(0xD9) != HAL_OK) return 0; // set pre-charge period
    if (SSD1306_WriteCommand(0x22) != HAL_OK) return 0;
    if (SSD1306_WriteCommand(0xDA) != HAL_OK) return 0; // set COM pins hardware config
    if (SSD1306_WriteCommand(0x12) != HAL_OK) return 0;
    if (SSD1306_WriteCommand(0xDB) != HAL_OK) return 0; // set vcomh
    if (SSD1306_WriteCommand(0x20) != HAL_OK) return 0; // 0.77xVcc
    if (SSD1306_WriteCommand(0x8D) != HAL_OK) return 0; // set DC-DC enable
    if (SSD1306_WriteCommand(0x14) != HAL_OK) return 0;
    if (SSD1306_WriteCommand(0xAF) != HAL_OK) return 0; // turn on panel

    /* Clear screen */
    SSD1306_Fill(Black);
    SSD1306_UpdateScreen();

    /* Set default values */
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;
    SSD1306.Initialized = 1;

    return 1;
}

void SSD1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void SSD1306_UpdateScreen(void) {
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
    for(uint8_t i = 0; i < SSD1306_HEIGHT / 8; i++) {
        SSD1306_WriteCommand(0xB0 + i); // Set the current RAM page address.
        SSD1306_WriteCommand(0x00);
        SSD1306_WriteCommand(0x10);
        HAL_I2C_Mem_Write(&hi2c1, SSD1306_ActiveAddress, 0x40, 1, &SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH, 100);
    }
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    if(SSD1306.Inverted) {
        color = (SSD1306_COLOR)!color;
    }

    if(color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

char SSD1306_WriteChar(char ch, sFONT font, SSD1306_COLOR color) {
    uint32_t i, b, j;

    // Check if character is valid
    if (ch < 32 || ch > 126)
        return 0;

    // Check remaining space on current line
    if (SSD1306_WIDTH < (SSD1306.CurrentX + font.Width) ||
        SSD1306_HEIGHT < (SSD1306.CurrentY + font.Height))
    {
        // Not enough space on current line
        return 0;
    }

    // Use the font table to draw the character
    for(i = 0; i < font.Height; i++) {
        b = font.table[(ch - 32) * font.Height + i];
        for(j = 0; j < font.Width; j++) {
            if((b << j) & 0x8000) {
                SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            } else {
                SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }

    // The current space is now taken
    SSD1306.CurrentX += font.Width;

    return ch;
}

char SSD1306_WriteString(char* str, sFONT font, SSD1306_COLOR color) {
    while (*str) {
        if (SSD1306_WriteChar(*str, font, color) != *str) {
            return *str; // Error
        }
        str++;
    }
    return *str;
}

void SSD1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}
