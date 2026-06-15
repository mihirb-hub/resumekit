#include "DFRobot_LCD.h" // Include the header file defining the DFRobot_LCD class
#include "freertos/FreeRTOS.h" // Include FreeRTOS base headers for multitasking support
#include "freertos/task.h" // Include FreeRTOS task headers for delay functions
#include "esp_log.h" // Include ESP32 logging library for error reporting

//#define I2C_MASTER_NUM      I2C_NUM_0
//#define I2C_MASTER_FREQ_HZ  100000
#define I2C_TIMEOUT_MS      1000 // Define a 1-second timeout for I2C communication operations

// Constructor for the DFRobot_LCD class initializing the I2C device handles
DFRobot_LCD::DFRobot_LCD(i2c_master_dev_handle_t lcdHandle,
       i2c_master_dev_handle_t rgbHandle)
   : _lcdHandle(lcdHandle), _rgbHandle(rgbHandle) {} // Use initializer list to assign handles to private members

static const char *LCD_TAG = "DFRobot_LCD"; // Define a static tag used for identification in logs
// Helper function to write a single byte of data to a specific register via I2C
esp_err_t DFRobot_LCD::i2c_write_byte(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t val)
{
   uint8_t data[2] = {reg, val}; // Create a data buffer containing the register address and the value
   esp_err_t ret = i2c_master_transmit(handle, data, sizeof(data), I2C_TIMEOUT_MS); // Execute the I2C transmission

   if (ret != ESP_OK){ // Check if the transmission returned an error code
       ESP_LOGE(LCD_TAG, "I2C write failed: %s", esp_err_to_name(ret)); // Log the error to the console if failed
   }
   return ret; // Return the status of the I2C operation
}

// Send a command byte to the LCD controller using the 0x80 control byte
void DFRobot_LCD::lcd_send_command(uint8_t cmd){
   i2c_write_byte(_lcdHandle, 0x80, cmd); // Write command byte to the LCD I2C handle
}

// Send a character data byte to the LCD RAM using the 0x40 control byte
void DFRobot_LCD::lcd_send_data(uint8_t data){
   i2c_write_byte(_lcdHandle, 0x40, data); // Write data byte to the LCD I2C handle
}

// Internal function to configure the RGB backlight driver registers
void DFRobot_LCD::rgb_set_color(uint8_t r, uint8_t g, uint8_t b){
   i2c_write_byte(_rgbHandle, 0x00, 0x00); // Initialize/reset the RGB driver mode register
   i2c_write_byte(_rgbHandle, 0x01, 0x20); // Configure PWM output settings for the driver
   i2c_write_byte(_rgbHandle, 0x08, 0xFF); // Set the output state to allow PWM control
   i2c_write_byte(_rgbHandle, 0x04, r); // Set the PWM duty cycle for the Red channel
   i2c_write_byte(_rgbHandle, 0x03, g); // Set the PWM duty cycle for the Green channel
   i2c_write_byte(_rgbHandle, 0x02, b); // Set the PWM duty cycle for the Blue channel
}

// Initialize the LCD hardware with the required power-on sequence and settings
void DFRobot_LCD::init(){
   vTaskDelay(pdMS_TO_TICKS(50)); // Wait 50ms to ensure the hardware is fully powered up

   lcd_send_command(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS); // Send function set: 2 lines, 5x8 font
   vTaskDelay(pdMS_TO_TICKS(5)); // Wait 5ms as per the controller's initialization spec
   lcd_send_command(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS); // Repeat function set to ensure sync
   vTaskDelay(pdMS_TO_TICKS(5)); // Wait another 5ms
   lcd_send_command(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS); // Third repetition of the function set

   lcd_send_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF); // Turn on display, hide cursor
   vTaskDelay(pdMS_TO_TICKS(1)); // Brief delay for display control processing

   lcd_send_command(LCD_CLEARDISPLAY); // Send command to clear all text from the screen
   vTaskDelay(pdMS_TO_TICKS(2)); // Clear command requires a longer delay (approx 2ms)

   lcd_send_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT); // Set text entry mode to increment to the left
   rgb_set_color(255, 255, 255); // Set the backlight to full white by default
}

// Clear all characters currently shown on the LCD screen
void DFRobot_LCD::clear(){
   lcd_send_command(LCD_CLEARDISPLAY); // Dispatch the clear command to the controller
   vTaskDelay(pdMS_TO_TICKS(2)); // Wait for the clear operation to complete
}

// Move the text cursor to a specific column and row position
void DFRobot_LCD::setCursor(uint8_t col, uint8_t row){
   uint8_t row_offsets[] = {0x00, 0x40}; // Define memory offsets for the start of row 0 and row 1
   if (row > 1) { // Check if the requested row is out of bounds
       row = 1; // Cap the row index to 1 (the second row)
   }
   lcd_send_command(LCD_SETDDRAMADDR | (col + row_offsets[row])); // Set the DDRAM address based on col and row
}

// Print a null-terminated string to the LCD screen
void DFRobot_LCD::printstr(const char* str){
   while(*str){ // Loop through each character in the string until the null terminator
       lcd_send_data((uint8_t)*str++); // Send the current character and increment the pointer
   }
}

// Public method to change the backlight color using RGB values (0-255)
void DFRobot_LCD::setRGB(uint8_t r, uint8_t g, uint8_t b){
   rgb_set_color(r, g, b); // Call the internal RGB setter function
}

