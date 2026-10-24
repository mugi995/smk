#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t led_effect;
    uint8_t led_brightness;
    uint8_t led_speed;
    uint8_t led_color;
    uint8_t ul_effect;
    uint8_t ul_brightness;
    uint8_t ul_speed;
    uint8_t battery_indicator_on;
    uint8_t rf_link;
    // Encoder keycodes: [layer][direction], 0 = the board default.
    uint16_t encoder[2][2];
    // Uniform RGB: 0 = off, 2 = solid; hue/sat/val (val 0..240).
    uint8_t rgb_mode;
    uint8_t rgb_hue;
    uint8_t rgb_sat;
    uint8_t rgb_val;
} user_settings_t;

extern user_settings_t user_settings;

// True when settings_load() found a valid record. Board init uses this to apply
// defaults only when nothing valid has been stored.
extern bool settings_loaded;

bool settings_load(void);

void settings_save(void);

void settings_mark_dirty(void);

void settings_task(void);

void settings_save_pre(void);
void settings_save_post(void);

#if DEBUG == 1
void settings_dump(void);
#endif
