
#include "DFRobot_LCD.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define I2C_SDA_PIN  7
#define I2C_SCL_PIN  8
#define I2C_FREQ_HZ  100000

DFRobot_RGBLCD1602::DFRobot_RGBLCD1602(uint8_t lcdCols, uint8_t lcdRows)
    : _cols(lcdCols), _rows(lcdRows),
      _bus_handle(nullptr), _lcd_handle(nullptr), _rgb_handle(nullptr) {}

void DFRobot_RGBLCD1602::i2c_send(i2c_master_dev_handle_t handle, uint8_t* data, int len) {
    i2c_master_transmit(handle, data, len, pdMS_TO_TICKS(100));
}

void DFRobot_RGBLCD1602::sendCommand(uint8_t val) {
    uint8_t buf[2] = {0x80, val};
    i2c_send(_lcd_handle, buf, 2);
}

void DFRobot_RGBLCD1602::sendData(uint8_t val) {
    uint8_t buf[2] = {0x40, val};
    i2c_send(_lcd_handle, buf, 2);
}

void DFRobot_RGBLCD1602::setReg(uint8_t addr, uint8_t val) {
    uint8_t buf[2] = {addr, val};
    i2c_send(_rgb_handle, buf, 2);
}

void DFRobot_RGBLCD1602::init() {
    // Init I2C bus
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = (gpio_num_t)I2C_SDA_PIN;
    bus_cfg.scl_io_num = (gpio_num_t)I2C_SCL_PIN;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    i2c_new_master_bus(&bus_cfg, &_bus_handle);

    // Add LCD device
    i2c_device_config_t lcd_cfg = {};
    lcd_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    lcd_cfg.device_address = LCD_ADDRESS;
    lcd_cfg.scl_speed_hz = I2C_FREQ_HZ;
    i2c_master_bus_add_device(_bus_handle, &lcd_cfg, &_lcd_handle);

    // Add RGB device
    i2c_device_config_t rgb_cfg = {};
    rgb_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    rgb_cfg.device_address = RGB_ADDRESS;
    rgb_cfg.scl_speed_hz = I2C_FREQ_HZ;
    i2c_master_bus_add_device(_bus_handle, &rgb_cfg, &_rgb_handle);

    vTaskDelay(pdMS_TO_TICKS(50));

    _showFunction = LCD_2LINE | LCD_5x8DOTS;
    _showControl  = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    _showMode     = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

    sendCommand(LCD_FUNCTIONSET | _showFunction);
    vTaskDelay(pdMS_TO_TICKS(5));
    sendCommand(LCD_FUNCTIONSET | _showFunction);
    vTaskDelay(pdMS_TO_TICKS(1));
    sendCommand(LCD_FUNCTIONSET | _showFunction);

    sendCommand(LCD_DISPLAYCONTROL | _showControl);
    clear();
    sendCommand(LCD_ENTRYMODESET | _showMode);

    // Init RGB backlight
    setReg(0x00, 0x00);
    setReg(0x01, 0x00);
    setReg(0x08, 0xAA);
    setColorWhite();
}

void DFRobot_RGBLCD1602::clear() {
    sendCommand(LCD_CLEARDISPLAY);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void DFRobot_RGBLCD1602::home() {
    sendCommand(LCD_RETURNHOME);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void DFRobot_RGBLCD1602::setCursor(uint8_t col, uint8_t row) {
    uint8_t row_offsets[] = {0x00, 0x40};
    sendCommand(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void DFRobot_RGBLCD1602::setRGB(const char* str) {
    setCursor(0, 0);
    printstr(str);
}

void DFRobot_RGBLCD1602::printstr(const char* str) {
    while (*str) {
        sendData((uint8_t)*str++);
    }
}

void DFRobot_RGBLCD1602::write(uint8_t data) {
    sendData(data);
}

void DFRobot_RGBLCD1602::command(uint8_t data) {
    sendCommand(data);
}

void DFRobot_RGBLCD1602::display() {
    _showControl |= LCD_DISPLAYON;
    sendCommand(LCD_DISPLAYCONTROL | _showControl);
}

void DFRobot_RGBLCD1602::noDisplay() {
    _showControl &= ~LCD_DISPLAYON;
    sendCommand(LCD_DISPLAYCONTROL | _showControl);
}

void DFRobot_RGBLCD1602::setColorWhite() {
    setRGBColor(255, 255, 255);
}

void DFRobot_RGBLCD1602::setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
    setReg(0x04, r);
    setReg(0x03, g);
    setReg(0x02, b);
}

void DFRobot_RGBLCD1602::setBacklight(bool mode) {
    if (mode) setColorWhite();
    else setRGBColor(0, 0, 0);
}


