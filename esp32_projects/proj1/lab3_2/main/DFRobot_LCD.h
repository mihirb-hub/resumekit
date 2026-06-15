#pragma once // Ensure the compiler includes this header only once during compilation
#include <cstdint> // Include standard integer types like uint8_t
#include "driver/i2c_master.h" // Include ESP-IDF I2C master driver definitions

#define LCD_ADDRESS      0x3E // Define the default I2C slave address for the LCD screen
#define RGB_ADDRESS      0x60 // Define the default I2C slave address for the RGB backlight controller

#define LCD_CLEARDISPLAY     0x01 // Command bitmask to clear the display screen
#define LCD_RETURNHOME       0x02 // Command bitmask to return cursor to the home position
#define LCD_ENTRYMODESET     0x04 // Command bitmask to set text entry mode
#define LCD_DISPLAYCONTROL   0x08 // Command bitmask for display, cursor, and blink control
#define LCD_FUNCTIONSET      0x20 // Command bitmask for setting interface, lines, and font
#define LCD_SETDDRAMADDR     0x80 // Command bitmask to set the Data Display RAM address

#define LCD_2LINE            0x08 // Flag to configure the LCD for 2-line display mode
#define LCD_5x8DOTS          0x00 // Flag to configure the character font to 5x8 dot matrix
#define LCD_DISPLAYON        0x04 // Flag to turn the LCD display visibility on
#define LCD_CURSOROFF        0x00 // Flag to keep the underline cursor hidden
#define LCD_BLINKOFF         0x00 // Flag to keep the cursor character blinking off
#define LCD_ENTRYLEFT        0x02 // Flag to set text direction from left to right

class DFRobot_LCD { // Define the class for interacting with the DFRobot LCD hardware
public:
   // Constructor that accepts I2C device handles for both the LCD and the RGB controller
   DFRobot_LCD(i2c_master_dev_handle_t lcdHandle, i2c_master_dev_handle_t rgbHandle);
   
   void init(); // Public method to initialize the LCD and its backlight
   void clear(); // Public method to wipe the screen and reset cursor position
   void setCursor(uint8_t col, uint8_t row); // Public method to move cursor to a specific column and row
   void printstr(const char* str); // Public method to display a string of text on the LCD
   void setRGB(uint8_t r, uint8_t g, uint8_t b); // Public method to set the backlight to a specific RGB color

private:
   i2c_master_dev_handle_t _lcdHandle; // Private member to store the LCD's I2C device handle
   i2c_master_dev_handle_t _rgbHandle; // Private member to store the RGB backlight's I2C device handle

   void lcd_send_command(uint8_t cmd); // Internal helper to send control instructions to the LCD
   void lcd_send_data(uint8_t data); // Internal helper to send character data to the LCD
   void rgb_set_color(uint8_t r, uint8_t g, uint8_t b); // Internal helper to update backlight driver registers
   // Internal utility to handle the low-level I2C byte writing to specific hardware registers
   esp_err_t i2c_write_byte(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t val);
};
