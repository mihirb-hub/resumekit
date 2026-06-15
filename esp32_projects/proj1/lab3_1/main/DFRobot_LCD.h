#pragma once
#include <stdint.h>
#include "driver/i2c_master.h"

#define LCD_ADDRESS     0x3E
#define RGB_ADDRESS     0x60

#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET    0x20
#define LCD_SETDDRAMADDR   0x80
#define LCD_SETCGRAMADDR   0x40

#define LCD_DISPLAYON  0x04
#define LCD_CURSOROFF  0x00
#define LCD_BLINKOFF   0x00
#define LCD_ENTRYLEFT  0x02
#define LCD_ENTRYSHIFTDECREMENT 0x00
#define LCD_2LINE      0x08
#define LCD_5x8DOTS    0x00

class DFRobot_RGBLCD1602 {
public:
    DFRobot_RGBLCD1602(uint8_t lcdCols=16, uint8_t lcdRows=2);
    void init();
    void clear();
    void home();
    void setCursor(uint8_t col, uint8_t row);
    void setRGB(const char* str);
    void printstr(const char* str);
    void display();
    void noDisplay();
    void setBacklight(bool mode);
    void setColorWhite();
    void command(uint8_t data);
    void write(uint8_t data);

private:
    uint8_t _cols, _rows;
    uint8_t _showControl, _showMode, _showFunction;

    i2c_master_bus_handle_t _bus_handle;
    i2c_master_dev_handle_t _lcd_handle;
    i2c_master_dev_handle_t _rgb_handle;

    void sendData(uint8_t val);
    void sendCommand(uint8_t val);
    void setRGBColor(uint8_t r, uint8_t g, uint8_t b);
    void setReg(uint8_t addr, uint8_t val);
    void i2c_send(i2c_master_dev_handle_t handle, uint8_t* data, int len);
};
