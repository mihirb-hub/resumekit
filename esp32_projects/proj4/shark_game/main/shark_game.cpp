/**
 * shark_game.cpp — ESP-IDF 6.x
 *
 * Shark Runner: one-button maze dodge game on DFRobot RGBLCD1602.
 * Hold BOOT (GPIO 9) to be on row 0, release to be on row 1.
 * Obstacles spawn randomly on either row — dodge by being on the other row.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"

#include "rgblcd1602.h"

static const char *TAG = "shark";

// ---------------------------------------------------------------------------
// Pin config
// ---------------------------------------------------------------------------
#define I2C_SDA  GPIO_NUM_7
#define I2C_SCL  GPIO_NUM_8
#define BOOT_BTN GPIO_NUM_9

// ---------------------------------------------------------------------------
// Game constants
// ---------------------------------------------------------------------------
#define LCD_COLS  16
#define LCD_ROWS  2

#define SHARK_COL       1
#define SHARK_ROW_FLOOR 1   // default row (BOOT released)
#define SHARK_ROW_AIR   0   // jump row   (BOOT held)

#define MAX_OBSTACLES  8    // more now since both rows used

#define SCROLL_PERIOD_START_MS  500
#define SCROLL_PERIOD_MIN_MS    180
#define SCROLL_SPEED_UP_SCORE   10

#define OBS_GAP_MIN  2      // slightly tighter gaps for more challenge
#define OBS_GAP_MAX  6

#define LIVES  3

// CGRAM slots
#define CGRAM_SHARK_BODY  0
#define CGRAM_OBSTACLE    1
#define CGRAM_HEART       2
#define CGRAM_WAVE        3

// ---------------------------------------------------------------------------
// Custom character bitmaps
// ---------------------------------------------------------------------------
static const uint8_t SHARK_HEAD[8] = {
    0b00000,
    0b00100,
    0b11110,
    0b11111,
    0b11110,
    0b00100,
    0b00000,
    0b00000,
};

static const uint8_t OBSTACLE_CHAR[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
};

static const uint8_t HEART_CHAR[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
};

static const uint8_t WAVE_CHAR[8] = {
    0b00000,
    0b00000,
    0b01010,
    0b10101,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
};

// ---------------------------------------------------------------------------
// Obstacle state — now has a row field
// ---------------------------------------------------------------------------
struct Obstacle {
    bool active;
    int  col;
    int  row;   // 0 = top row, 1 = bottom row
};

static Obstacle obs[MAX_OBSTACLES];

static void obs_reset(void)
{
    memset(obs, 0, sizeof(obs));
}

static void obs_spawn(void)
{
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obs[i].active) {
            obs[i].active = true;
            obs[i].col    = LCD_COLS - 1;
            // Randomly pick top or bottom row
            obs[i].row    = (int)(esp_random() % 2);
            return;
        }
    }
}

static void obs_scroll(void)
{
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obs[i].active) continue;
        obs[i].col--;
        if (obs[i].col < 0)
            obs[i].active = false;
    }
}

// Check if any obstacle is at SHARK_COL on the given row
static bool obs_collision_on_row(int row)
{
    for (int i = 0; i < MAX_OBSTACLES; i++)
        if (obs[i].active && obs[i].col == SHARK_COL && obs[i].row == row)
            return true;
    return false;
}

// Closest obstacle on a specific row (for warning color)
static int obs_closest_on_row(int row)
{
    int closest = 99;
    for (int i = 0; i < MAX_OBSTACLES; i++)
        if (obs[i].active && obs[i].row == row && obs[i].col > SHARK_COL && obs[i].col < closest)
            closest = obs[i].col;
    return closest;
}

// ---------------------------------------------------------------------------
// Button
// ---------------------------------------------------------------------------
static void btn_init(void)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << BOOT_BTN);
    cfg.mode         = GPIO_MODE_INPUT;
    cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
}

static bool btn_held(void)
{
    return gpio_get_level(BOOT_BTN) == 0;
}

static void btn_wait_press_release(void)
{
    while (!btn_held())  vTaskDelay(pdMS_TO_TICKS(10));
    while (btn_held())   vTaskDelay(pdMS_TO_TICKS(10));
}

// ---------------------------------------------------------------------------
// Backlight colours
// ---------------------------------------------------------------------------
static void color_normal(void)   { lcd_set_color(0,   220, 60);  }
static void color_warning(void)  { lcd_set_color(255, 160, 0);   }
static void color_danger(void)   { lcd_set_color(255, 0,   0);   }
static void color_title(void)    { lcd_set_color(0,   180, 255); }
static void color_gameover(void) { lcd_set_color(200, 0,   80);  }

static void color_hit_flash(void)
{
    for (int i = 0; i < 3; i++) {
        color_danger();
        vTaskDelay(pdMS_TO_TICKS(120));
        lcd_set_color(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
    color_normal();
}

// ---------------------------------------------------------------------------
// Draw — both rows can now have obstacles and the shark
// ---------------------------------------------------------------------------
static void draw_frame(int shark_row, int score, int lives)
{
    for (int row = 0; row < LCD_ROWS; row++) {
        lcd_set_cursor(0, row);
        for (int col = 0; col < LCD_COLS; col++) {

            // Shark — always 3 chars wide: [glyph][~][>]
            if (row == shark_row) {
                if      (col == SHARK_COL)     { lcd_write_char((char)CGRAM_SHARK_BODY); continue; }
                else if (col == SHARK_COL + 1) { lcd_write_char('~'); continue; }
                else if (col == SHARK_COL + 2) { lcd_write_char('>'); continue; }
            }

            // Obstacle on this row/col?
            bool has_obs = false;
            for (int i = 0; i < MAX_OBSTACLES; i++)
                if (obs[i].active && obs[i].col == col && obs[i].row == row)
                    { has_obs = true; break; }
            if (has_obs) { lcd_write_char((char)CGRAM_OBSTACLE); continue; }

            // Score/lives HUD — top-right of row 0 when shark is on floor
            // Only fill HUD if this cell isn't occupied by shark or obstacle
            if (row == 0 && shark_row == SHARK_ROW_FLOOR) {
                // Right 6 chars of row 0: " S:000"
                if (col == LCD_COLS - 6) {
                    char s[7];
                    snprintf(s, sizeof(s), " S:%03d", score);
                    lcd_write_string(s);
                    break; // rest of row handled by snprintf
                }
                // Hearts on left of row 0
                if (col < LIVES) {
                    lcd_write_char(col < lives ? (char)CGRAM_HEART : ' ');
                    continue;
                }
            }

            // Background filler
            lcd_write_char((col % 4 == 0) ? (char)CGRAM_WAVE : ' ');
        }
    }
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------
static void screen_title(void)
{
    color_title();
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_write_string(">~~> SHARK RUN");
    lcd_set_cursor(0, 1);
    lcd_write_string("Hold BOOT 2 dodge");
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_write_string("   Press BOOT to  ");
    lcd_set_cursor(0, 1);
    lcd_write_string("     START!     ");
}

static void screen_gameover(int score)
{
    color_gameover();
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_write_string("    GAME  OVER!   ");
    char buf[17];
    snprintf(buf, sizeof(buf), " Score: %04d    ", score);
    lcd_set_cursor(0, 1);
    lcd_write_string(buf);
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd_set_cursor(0, 1);
    lcd_write_string(" BOOT to retry  ");
}

// ---------------------------------------------------------------------------
// Game loop
// ---------------------------------------------------------------------------
static int run_game(void)
{
    obs_reset();

    int score             = 0;
    int lives             = LIVES;
    int shark_row         = SHARK_ROW_FLOOR;
    int scroll_ms         = SCROLL_PERIOD_START_MS;
    int ticks_since_spawn = 0;
    int next_spawn_gap    = OBS_GAP_MIN + (int)(esp_random() % (OBS_GAP_MAX - OBS_GAP_MIN + 1));

    color_normal();

    while (lives > 0) {

        // Scroll and spawn
        obs_scroll();
        ticks_since_spawn++;
        if (ticks_since_spawn >= next_spawn_gap) {
            obs_spawn();
            ticks_since_spawn = 0;
            next_spawn_gap = OBS_GAP_MIN + (int)(esp_random() % (OBS_GAP_MAX - OBS_GAP_MIN + 1));
            score++;
        }

        // Speed up
        int speed_tier = score / SCROLL_SPEED_UP_SCORE;
        scroll_ms = SCROLL_PERIOD_START_MS - speed_tier * 40;
        if (scroll_ms < SCROLL_PERIOD_MIN_MS) scroll_ms = SCROLL_PERIOD_MIN_MS;

        // Collision — check shark's current row
        if (obs_collision_on_row(shark_row)) {
            lives--;
            // Remove the killing obstacle
            for (int i = 0; i < MAX_OBSTACLES; i++)
                if (obs[i].active && obs[i].col == SHARK_COL && obs[i].row == shark_row)
                    obs[i].active = false;
            color_hit_flash();
            if (lives <= 0) break;
        }

        // Warning: obstacle close on shark's current row
        int closest = obs_closest_on_row(shark_row);
        if (closest <= 4)
            color_warning();
        else
            color_normal();

        draw_frame(shark_row, score, lives);

        // Wait scroll_ms, polling button every 20ms
        int64_t tick_end = esp_timer_get_time() + (int64_t)scroll_ms * 1000;
        while (esp_timer_get_time() < tick_end) {
            shark_row = btn_held() ? SHARK_ROW_AIR : SHARK_ROW_FLOOR;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    return score;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Shark Runner starting...");

    btn_init();

    // I2C master bus
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port          = I2C_NUM_0;
    bus_cfg.sda_io_num        = I2C_SDA;
    bus_cfg.scl_io_num        = I2C_SCL;
    bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    lcd_init(bus);

    // Load custom characters
    lcd_define_char(CGRAM_SHARK_BODY, SHARK_HEAD);
    lcd_define_char(CGRAM_OBSTACLE,   OBSTACLE_CHAR);
    lcd_define_char(CGRAM_HEART,      HEART_CHAR);
    lcd_define_char(CGRAM_WAVE,       WAVE_CHAR);

    // I2C Bus Scanner
    ESP_LOGI(TAG, "--- Scanning I2C bus ---");
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_master_dev_handle_t tmp;
        i2c_device_config_t tmp_cfg = {};
        tmp_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        tmp_cfg.device_address  = addr;
        tmp_cfg.scl_speed_hz    = 100000;
        if (i2c_master_bus_add_device(bus, &tmp_cfg, &tmp) == ESP_OK) {
            uint8_t dummy;
            if (i2c_master_receive(tmp, &dummy, 1, 100) == ESP_OK)
                ESP_LOGI(TAG, "Found device at 0x%02X", addr);
            i2c_master_bus_rm_device(tmp);
        }
    }
    ESP_LOGI(TAG, "--- Scan complete ---");

    // Main loop
    while (true) {
        screen_title();
        btn_wait_press_release();

        int score = run_game();

        screen_gameover(score);
        btn_wait_press_release();
    }
}

