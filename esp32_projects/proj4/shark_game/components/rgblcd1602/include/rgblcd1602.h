#pragma once
/**
 * rgblcd1602.h
 *
 * Driver for the DFRobot LCD1602 RGB backlight display.
 *
 * Two I2C devices on the same bus:
 *   LCD controller  — AiP31068  @ 0x7C  (7-bit)
 *   RGB backlight   — PCA9633   @ 0x2D  (7-bit)
 *
 * 16 columns × 2 rows, character-cell display.
 */

#include <stdint.h>
#include "driver/i2c_master.h"

// Default I2C addresses (DFRobot standard)
#define LCD_I2C_ADDR  0x3E
#define RGB_I2C_ADDR  0x2D

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise both the LCD controller and the RGB backlight.
 * Call once after creating an I2C master bus.
 */
void lcd_init(i2c_master_bus_handle_t bus);

/** Turn the RGB backlight on with the given colour (0–255 each channel). */
void lcd_set_color(uint8_t r, uint8_t g, uint8_t b);

/** Clear all characters and return cursor to home (0,0). */
void lcd_clear(void);

/** Move cursor to column col (0–15), row row (0–1). */
void lcd_set_cursor(uint8_t col, uint8_t row);

/** Write a single ASCII character at the current cursor position. */
void lcd_write_char(char c);

/** Write a null-terminated string from the current cursor position. */
void lcd_write_string(const char *s);

/**
 * Define a custom character in CGRAM.
 * slot  : 0–7
 * bitmap: 8 bytes, each byte is one row (bits 4–0 used, MSB = left)
 */
void lcd_define_char(uint8_t slot, const uint8_t bitmap[8]);

#ifdef __cplusplus
}
#endif

