#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// I2C Configuration for UCSC C3 Rust Board
#define I2C_MASTER_SCL_IO           10
#define I2C_MASTER_SDA_IO           8
#define I2C_MASTER_FREQ_HZ          100000
#define SHTC3_ADDR                  0x70

// SHTC3 Commands
#define SHTC3_CMD_WAKEUP            0x3517
#define SHTC3_CMD_SLEEP             0xB098
#define SHTC3_CMD_MEAS_T_FIRST      0x7866
#define SHTC3_CMD_MEAS_H_FIRST      0x58E0

static const char *TAG = "Lab2_2_SHTC3";

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

// --- CRC-8 Calculation ---
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

// --- I2C Initialization ---
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

// --- Helper to send 16-bit commands ---
esp_err_t shtc3_send_command(uint16_t cmd) {
    uint8_t cmd_bytes[2] = { (cmd >> 8) & 0xFF, cmd & 0xFF };
    return i2c_master_transmit(dev_handle, cmd_bytes, 2, 100);
}

float read_temperature() {
    uint8_t cmd[2] = { 0x78, 0x66 };
    uint8_t data[3] = {0};

    esp_err_t err = i2c_master_transmit(dev_handle, cmd, 2, 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Temp transmit failed: %s", esp_err_to_name(err));
        return -999.0;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    err = i2c_master_receive(dev_handle, data, 3, 100);
    ESP_LOGI(TAG, "Temp raw: 0x%02X 0x%02X 0x%02X", data[0], data[1], data[2]);
    ESP_LOGI(TAG, "Temp CRC calc: 0x%02X, expected: 0x%02X", calculate_crc(data, 2), data[2]);

    if (err != ESP_OK || calculate_crc(data, 2) != data[2]) {
        ESP_LOGE(TAG, "Temp read failed: %s", esp_err_to_name(err));
        return -999.0;
    }
    uint16_t raw_val = (data[0] << 8) | data[1];
    return -45.0 + 175.0 * ((float)raw_val / 65536.0);
}

float read_humidity() {
    uint8_t cmd[2] = { 0x58, 0xE0 };
    uint8_t data[3] = {0};

    esp_err_t err = i2c_master_transmit(dev_handle, cmd, 2, 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Hum transmit failed: %s", esp_err_to_name(err));
        return -999.0;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    err = i2c_master_receive(dev_handle, data, 3, 100);
    ESP_LOGI(TAG, "Hum raw: 0x%02X 0x%02X 0x%02X", data[0], data[1], data[2]);
    ESP_LOGI(TAG, "Hum CRC calc: 0x%02X, expected: 0x%02X", calculate_crc(data, 2), data[2]);

    if (err != ESP_OK || calculate_crc(data, 2) != data[2]) {
        ESP_LOGE(TAG, "Hum read failed: %s", esp_err_to_name(err));
        return -999.0;
    }
    uint16_t raw_val = (data[0] << 8) | data[1];
    return 100.0 * ((float)raw_val / 65536.0);
}

void app_main(void) {
    i2c_master_init();
    ESP_LOGI(TAG, "SHTC3 Driver Started");

    // Scan I2C bus to find devices
    ESP_LOGI(TAG, "Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t res = i2c_master_probe(bus_handle, addr, 50);
        if (res == ESP_OK) {
            ESP_LOGI(TAG, "Found device at address: 0x%02X", addr);
        }
    }
    ESP_LOGI(TAG, "Scan complete");

    while (1) {
        shtc3_send_command(SHTC3_CMD_WAKEUP);
        vTaskDelay(pdMS_TO_TICKS(10)); // increased from 1ms

        float temp_c = read_temperature();
        float hum    = read_humidity();
        float temp_f = (temp_c * 9.0 / 5.0) + 32.0;

        shtc3_send_command(SHTC3_CMD_SLEEP);

        if (temp_c != -999.0 && hum != -999.0) {
            printf("Temperature is %.0fC (or %.0fF) with a %.0f%% humidity\n",
                   temp_c, temp_f, hum);
        } else {
            ESP_LOGE(TAG, "Read failed (CRC error or I2C error)");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


