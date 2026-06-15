#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "hid_dev.h"

#define HID_DEMO_TAG        "HID_DEMO"
#define HIDD_DEVICE_NAME    "HID"

// I2C pins
#define I2C_SCL_IO          8
#define I2C_SDA_IO          7
#define I2C_FREQ_HZ         400000

// ICM-42670-P
#define ICM_ADDR            0x68
#define REG_PWR_MGMT0       0x1F
#define REG_ACCEL_DATA_X1   0x0B

// Inclination thresholds
#define THRESHOLD_BIT       500
#define THRESHOLD_LOT       2000

// Base mouse deltas
#define DELTA_BIT           5
#define DELTA_LOT           15

// Boot button for click
#define BOOT_BUTTON_GPIO    9

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

static uint8_t hidd_service_uuid128[] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = ESP_BLE_GAP_CONN_ITVL_MS(7.5),
    .max_interval = ESP_BLE_GAP_CONN_ITVL_MS(20),
    .appearance = 0x03c0,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min        = ESP_BLE_GAP_ADV_ITVL_MS(20),
    .adv_int_max        = ESP_BLE_GAP_ADV_ITVL_MS(30),
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// ---- I2C / IMU ----

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

// ---- BLE callbacks ----

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch(event) {
        case ESP_HIDD_EVENT_REG_FINISH: {
            if (param->init_finish.state == ESP_HIDD_INIT_OK) {
                esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
                esp_ble_gap_config_adv_data(&hidd_adv_data);
            }
            break;
        }
        case ESP_BAT_EVENT_REG:
            break;
        case ESP_HIDD_EVENT_DEINIT_FINISH:
            break;
        case ESP_HIDD_EVENT_BLE_CONNECT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
            hid_conn_id = param->connect.conn_id;
            break;
        }
        case ESP_HIDD_EVENT_BLE_DISCONNECT: {
            sec_conn = false;
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        }
        case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
            ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->vendor_write.data, param->vendor_write.length);
            break;
        }
        case ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT");
            ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->led_write.data, param->led_write.length);
            break;
        }
        default:
            break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            sec_conn = true;
            ESP_LOGI(HID_DEMO_TAG, "secure connection established.");
        } else {
            ESP_LOGE(HID_DEMO_TAG, "pairing failed, reason = 0x%x",
                     param->ble_security.auth_cmpl.fail_reason);
        }
        break;
    default:
        break;
    }
}

// ---- HID task ----

void hid_demo_task(void *pvParameters)
{
    // setup boot button as input for click
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // acceleration state
    int64_t left_start_time = 0;
    int64_t right_start_time = 0;
    int64_t up_start_time = 0;
    int64_t down_start_time = 0;

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1) {
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (!sec_conn) continue;

        int16_t ax, ay, az;
        icm_read_accel(&ax, &ay, &az);

        int8_t x_move = 0;
        int8_t y_move = 0;
        int64_t now = esp_timer_get_time() / 1000; // convert us to ms

        // ---- X axis (LEFT/RIGHT) ----
        if (ax < -THRESHOLD_LOT) {
            if (left_start_time == 0) left_start_time = now;
            int64_t elapsed = now - left_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            x_move = -(DELTA_LOT * a);
            right_start_time = 0;
        } else if (ax < -THRESHOLD_BIT) {
            if (left_start_time == 0) left_start_time = now;
            int64_t elapsed = now - left_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            x_move = -(DELTA_BIT * a);
            right_start_time = 0;
        } else if (ax > THRESHOLD_LOT) {
            if (right_start_time == 0) right_start_time = now;
            int64_t elapsed = now - right_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            x_move = (DELTA_LOT * a);
            left_start_time = 0;
        } else if (ax > THRESHOLD_BIT) {
            if (right_start_time == 0) right_start_time = now;
            int64_t elapsed = now - right_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            x_move = (DELTA_BIT * a);
            left_start_time = 0;
        } else {
            left_start_time = 0;
            right_start_time = 0;
        }

        // ---- Y axis (UP/DOWN) ----
        if (ay > THRESHOLD_LOT) {
            if (up_start_time == 0) up_start_time = now;
            int64_t elapsed = now - up_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            y_move = -(DELTA_LOT * a);
            down_start_time = 0;
        } else if (ay > THRESHOLD_BIT) {
            if (up_start_time == 0) up_start_time = now;
            int64_t elapsed = now - up_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            y_move = -(DELTA_BIT * a);
            down_start_time = 0;
        } else if (ay < -THRESHOLD_LOT) {
            if (down_start_time == 0) down_start_time = now;
            int64_t elapsed = now - down_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            y_move = (DELTA_LOT * a);
            up_start_time = 0;
        } else if (ay < -THRESHOLD_BIT) {
            if (down_start_time == 0) down_start_time = now;
            int64_t elapsed = now - down_start_time;
            int a = (elapsed > 50) ? 3 : (elapsed > 10) ? 2 : 1;
            y_move = (DELTA_BIT * a);
            up_start_time = 0;
        } else {
            up_start_time = 0;
            down_start_time = 0;
        }

        // ---- Send mouse movement ----
        if (x_move != 0 || y_move != 0) {
            esp_hidd_send_mouse_value(hid_conn_id, 0, x_move, y_move);
        }

        // ---- Boot button click ----
        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
            ESP_LOGI(HID_DEMO_TAG, "Click!");
            esp_hidd_send_mouse_value(hid_conn_id, 1, 0, 0);  // button down
            vTaskDelay(50 / portTICK_PERIOD_MS);
            esp_hidd_send_mouse_value(hid_conn_id, 0, 0, 0);  // button up
            vTaskDelay(200 / portTICK_PERIOD_MS);              // debounce
        }
    }
}

void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2c_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    icm_init();

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed", __func__);
        return;
    }

    esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&cfg);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return;
    }

    if ((ret = esp_hidd_profile_init()) != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "%s init hidd failed", __func__);
    }

    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    xTaskCreate(&hid_demo_task, "hid_task", 2048, NULL, 5, NULL);
}
