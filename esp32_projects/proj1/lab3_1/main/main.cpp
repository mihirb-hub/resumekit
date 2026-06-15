#include "DFRobot_LCD.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
    DFRobot_RGBLCD1602 lcd;
    lcd.init();

    while (true) {
        lcd.setRGB("Hello CSE121!");
        lcd.setCursor(0, 1);
        lcd.printstr("Bhagatwala");  // change to your actual last name
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
