/**
 * rgblcd1602.c
 *
 * DFRobot LCD1602 RGB driver for ESP-IDF (new I2C master API).
 *
 * LCD controller: AiP31068 (HD44780-compatible over I2C)
 * RGB controller: PCA9633  (4-channel LED driver)
 */

#include "rgblcd1602.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "rgblcd";

// ---- LCD command bytes ----
#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET    0x20
#define LCD_SETCGRAMADDR   0x40
#define LCD_SETDDRAMADDR   0x80

// Entry mode flags
#define LCD_ENTRYLEFT      0x02

// Display control flags
#define LCD_DISPLAYON      0x04
#define LCD_CURSOROFF      0x00
#define LCD_BLINKOFF       0x00

// Function set flags
#define LCD_2LINE          0x08
#define LCD_5x8DOTS        0x00

// PCA9633 registers
#define PCA9633_MODE1      0x00
#define PCA9633_MODE2      0x01
#define PCA9633_PWM0       0x02   // Blue
#define PCA9633_PWM1       0x03   // Green
#define PCA9633_PWM2       0x04   // Red
#define PCA9633_LEDOUT     0x08

// I2C device handles
static i2c_master_dev_handle_t s_lcd = NULL;
static i2c_master_dev_handle_t s_rgb = NULL;

// ---------------------------------------------------------------------------
// Low-level send helpers
// ---------------------------------------------------------------------------

static void lcd_send_cmd(uint8_t cmd)
{
    // AiP31068: first byte 0x80 = Co=1,Rs=0 (command)
    uint8_t buf[2] = {0x80, cmd};
    esp_err_t ret = i2c_master_transmit(s_lcd, buf, 2, 100);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "lcd cmd 0x%02X err: %s", cmd, esp_err_to_name(ret));
    vTaskDelay(pdMS_TO_TICKS(2));
}

static void lcd_send_data(uint8_t data)
{
    // AiP31068: first byte 0x40 = Co=0,Rs=1 (data / CGRAM or DDRAM write)
    uint8_t buf[2] = {0x40, data};
    esp_err_t ret = i2c_master_transmit(s_lcd, buf, 2, 100);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "lcd data 0x%02X err: %s", data, esp_err_to_name(ret));
}

static void rgb_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    esp_err_t ret = i2c_master_transmit(s_rgb, buf, 2, 100);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "rgb reg 0x%02X err: %s", reg, esp_err_to_name(ret));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void lcd_init(i2c_master_bus_handle_t bus)
{
    // Add LCD device
    i2c_device_config_t lcd_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = LCD_I2C_ADDR,
        .scl_speed_hz    = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &lcd_cfg, &s_lcd));

    // Add RGB device
    i2c_device_config_t rgb_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = RGB_I2C_ADDR,
        .scl_speed_hz    = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &rgb_cfg, &s_rgb));

    // LCD init sequence (HD44780 compatible)
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_send_cmd(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_cmd(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_send_cmd(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS);

    lcd_send_cmd(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
    lcd_send_cmd(LCD_CLEARDISPLAY);
    vTaskDelay(pdMS_TO_TICKS(3));
    lcd_send_cmd(LCD_ENTRYMODESET | LCD_ENTRYLEFT);

    // RGB init (PCA9633)
    rgb_write_reg(PCA9633_MODE1,  0x00); // normal mode
    rgb_write_reg(PCA9633_MODE2,  0x20); // DMBLNK = totem pole
    rgb_write_reg(PCA9633_LEDOUT, 0xAA); // all channels in PWM mode
    lcd_set_color(0, 255, 0);            // default: green

    ESP_LOGI(TAG, "LCD1602 RGB ready");
}

void lcd_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    rgb_write_reg(PCA9633_PWM2, r);
    rgb_write_reg(PCA9633_PWM1, g);
    rgb_write_reg(PCA9633_PWM0, b);
}

void lcd_clear(void)
{
    lcd_send_cmd(LCD_CLEARDISPLAY);
    vTaskDelay(pdMS_TO_TICKS(3));
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    // Row 0 base = 0x00, Row 1 base = 0x40
    uint8_t row_offsets[] = {0x00, 0x40};
    lcd_send_cmd(LCD_SETDDRAMADDR | (col + row_offsets[row & 0x01]));
}

void lcd_write_char(char c)
{
    lcd_send_data((uint8_t)c);
}

void lcd_write_string(const char *s)
{
    while (*s) lcd_write_char(*s++);
}

void lcd_define_char(uint8_t slot, const uint8_t bitmap[8])
{
    lcd_send_cmd(LCD_SETCGRAMADDR | ((slot & 0x07) << 3));
    for (int i = 0; i < 8; i++)
        lcd_send_data(bitmap[i]);
    // Return to DDRAM mode
    lcd_send_cmd(LCD_SETDDRAMADDR | 0x00);
}



