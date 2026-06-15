#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

// I2C / SHTC3 (onboard)
#define I2C_MASTER_SCL_IO           8
#define I2C_MASTER_SDA_IO           7
#define I2C_MASTER_FREQ_HZ          100000
#define SHTC3_ADDR                  0x70
#define SHTC3_CMD_WAKEUP            0x3517
#define SHTC3_CMD_SLEEP             0xB098

// US-100 pins
#define TRIG_PIN    GPIO_NUM_5
#define ECHO_PIN    GPIO_NUM_4

static const char *TAG = "Lab5";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

// --- CRC-8 ---
uint8_t calculate_crc(uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else crc <<= 1;
        }
    }
    return crc;
}

// --- I2C Init ---
void i2c_master_init() {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
}

esp_err_t shtc3_send_command(uint16_t cmd) {
    uint8_t cmd_bytes[2] = { (cmd >> 8) & 0xFF, cmd & 0xFF };
    return i2c_master_transmit(dev_handle, cmd_bytes, 2, 1000);
}

float read_temperature() {
    // Wake up, send measure command, read result
    shtc3_send_command(SHTC3_CMD_WAKEUP);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t cmd[2] = { 0x7C, 0xA2 };
    i2c_master_transmit(dev_handle, cmd, 2, 1000);
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t data[6] = {0};
    i2c_master_receive(dev_handle, data, 6, 1000);

    // bytes 0,1 = temp, byte 2 = temp CRC
    // bytes 3,4 = hum,  byte 5 = hum CRC
    if (calculate_crc(data, 2) != data[2]) {
        ESP_LOGE(TAG, "Temp CRC error, using fallback 23C");
        return 23.0f;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    return -45.0f + 175.0f * ((float)raw / 65536.0f);
}

// --- GPIO Init ---
void init_gpio() {
    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << TRIG_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&trig_conf);

    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&echo_conf);

    gpio_set_level(TRIG_PIN, 0);
}

// --- US-100 Ranging ---
float measure_distance(float temp_c) {
    float speed_of_sound = 331.3f + 0.606f * temp_c; // m/s

    // 10us trigger pulse
    gpio_set_level(TRIG_PIN, 0);
    ets_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    ets_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    // Wait for echo HIGH
    int32_t timeout = 30000;
    while (gpio_get_level(ECHO_PIN) == 0) {
        ets_delay_us(1);
        if (--timeout <= 0) return -1.0f;
    }

    // Measure echo pulse width
    int32_t duration_us = 0;
    timeout = 30000;
    while (gpio_get_level(ECHO_PIN) == 1) {
        ets_delay_us(1);
        duration_us++;
        if (--timeout <= 0) return -1.0f;
    }

    // round trip so divide by 2
    float distance_cm = ((float)duration_us / 1000000.0f) * speed_of_sound * 100.0f / 2.0f;
    return distance_cm;
}

void app_main(void) {
    i2c_master_init();
    init_gpio();
    ESP_LOGI(TAG, "Lab5 started");

    // I2C scan
    ESP_LOGI(TAG, "Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t res = i2c_master_probe(bus_handle, addr, 50);
        if (res == ESP_OK) {
            ESP_LOGI(TAG, "Found device at address: 0x%02X", addr);
        }
    }
    ESP_LOGI(TAG, "Scan complete");

    while (1) {
        float temp_c = read_temperature();
        shtc3_send_command(SHTC3_CMD_SLEEP);

        // Average 5 distance readings
        float total = 0.0f;
        int valid = 0;
        for (int i = 0; i < 5; i++) {
            float d = measure_distance(temp_c);
            if (d > 0.0f) {
                total += d;
                valid++;
            }
            ets_delay_us(10000); // 10ms between pulses
        }

        if (valid > 0) {
            float dist = total / valid;
            printf("Distance: %.1fcm at %.0fC\n", dist, temp_c);
        } else {
            printf("Distance: timeout\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


