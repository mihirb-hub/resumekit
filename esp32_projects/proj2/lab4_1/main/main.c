#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "lab4_1";

// I2C pins (ESP32-C3 Rust board)
#define I2C_SCL_IO          8
#define I2C_SDA_IO          7
#define I2C_FREQ_HZ         400000

// ICM-42670-P
#define ICM_ADDR            0x68
#define REG_PWR_MGMT0       0x1F
#define REG_ACCEL_DATA_X1   0x0B

// Tilt threshold (~0.3g at ±2g range, raw 16-bit)
#define THRESHOLD           500

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

// ---- Init ----

static void i2c_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ICM_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

static void icm_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, buf, 2, pdMS_TO_TICKS(100)));
}

static void icm_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, pdMS_TO_TICKS(100)));
}

static void icm_init(void) {
    icm_write_reg(REG_PWR_MGMT0, 0x0F);
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void icm_read_accel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t raw[6];
    icm_read_regs(REG_ACCEL_DATA_X1, raw, 6);
    *ax = (int16_t)((raw[0] << 8) | raw[1]);
    *ay = (int16_t)((raw[2] << 8) | raw[3]);
    *az = (int16_t)((raw[4] << 8) | raw[5]);
}

// ---- Main ----

void app_main(void) {
    i2c_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    icm_init();

    int16_t ax, ay, az;

    while (1) {
        icm_read_accel(&ax, &ay, &az);

        // Debug: uncomment to see raw values and tune threshold
 //       ESP_LOGI(TAG, "raw ax=%d ay=%d az=%d", ax, ay, az);

        char x_dir[8] = "";
        char y_dir[8] = "";

        if      (ax >  THRESHOLD) strcpy(x_dir, "RIGHT");
        else if (ax < -THRESHOLD) strcpy(x_dir, "LEFT");

        if      (ay >  THRESHOLD) strcpy(y_dir, "UP");
        else if (ay < -THRESHOLD) strcpy(y_dir, "DOWN");

        if (strlen(x_dir) && strlen(y_dir)) {
            ESP_LOGI(TAG, "%s %s", y_dir, x_dir);  // e.g. "UP LEFT"
        } else if (strlen(y_dir)) {
            ESP_LOGI(TAG, "%s", y_dir);             // e.g. "UP" or "DOWN"
        } else if (strlen(x_dir)) {
            ESP_LOGI(TAG, "%s", x_dir);             // e.g. "LEFT" or "RIGHT"
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
