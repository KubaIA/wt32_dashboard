#include "screen_page3.h"
#include "ui_common.h"

#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t lv_montserrat_60;

#ifdef __cplusplus
}
#endif

#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Weather icons ---- */
#include "pictures/img_00_clear_day.h"
#include "pictures/img_00_clear_night.h"
#include "pictures/img_01_mainly_clear_day.h"
#include "pictures/img_01_mainly_clear_night.h"
#include "pictures/img_02_partly_clear_day.h"
#include "pictures/img_02_partly_clear_night.h"
#include "pictures/img_03_overcast.h"
#include "pictures/img_45_fog.h"
#include "pictures/img_61_rain_light_day.h"
#include "pictures/img_61_rain_light_night.h"
#include "pictures/img_63_rain_medium.h"
#include "pictures/img_65_rain_heavy.h"
#include "pictures/img_66_sleet.h"
#include "pictures/img_71_snow.h"
#include "pictures/img_95_thunderstorm.h"
#include "pictures/img_96_thunderstorm_hail.h"
#include "pictures/img_unknown.h"

/* ---- Weather code -> english text ---- */
static const char* weathercode_to_text(int code) {
    switch (code) {
        case 0:  return "Clear";
        case 1:  return "Mostly Clear";
        case 2:  return "Partly Cloudy";
        case 3:  return "Overcast";
        case 45: return "Fog";
        case 48: return "Freezing Fog";
        case 51: return "Light Drizzle";
        case 53: return "Moderate Drizzle";
        case 55: return "Heavy Drizzle";
        case 56: return "Light Freezing Drizzle";
        case 57: return "Freezing Drizzle";
        case 61: return "Light Rain";
        case 63: return "Moderate Rain";
        case 65: return "Heavy Rain";
        case 66: return "Light Freezing Rain";
        case 67: return "Freezing Rain";
        case 71: return "Light Snow";
        case 73: return "Snow";
        case 75: return "Heavy Snow";
        case 77: return "Snow Grains";
        case 80: return "Light Rain Shower";
        case 81: return "Rain Shower";
        case 82: return "Heavy Rain Shower";
        case 85: return "Snow Shower";
        case 86: return "Heavy Snow Shower";
        case 95: return "Thunderstorm";
        case 96: return "Thunderstorm with Hail";
        case 99: return "Severe Thunderstorm";
        default: return "Unknown";
    }
}

/* ---- Weather code -> icon ---- */
static const lv_image_dsc_t* weathercode_to_icon(int code, bool is_day) {
    switch (code) {
        case 0:  return is_day ? &img_00_clear_day : &img_00_clear_night;
        case 1:  return is_day ? &img_01_mainly_clear_day : &img_01_mainly_clear_night;
        case 2:  return is_day ? &img_02_partly_clear_day : &img_02_partly_clear_night;
        case 3:  return &img_03_overcast;
        case 45: return &img_45_fog;
        case 48: return &img_45_fog;
        case 51: return is_day ? &img_61_rain_light_day : &img_61_rain_light_night;
        case 53: return &img_63_rain_medium;
        case 55: return &img_65_rain_heavy;
        case 56: return &img_66_sleet;
        case 57: return &img_66_sleet;
        case 61: return is_day ? &img_61_rain_light_day : &img_61_rain_light_night;
        case 63: return &img_63_rain_medium;
        case 65: return &img_65_rain_heavy;
        case 66: return &img_66_sleet;
        case 67: return &img_66_sleet;
        case 71: return &img_71_snow;
        case 73: return &img_71_snow;
        case 75: return &img_71_snow;
        case 77: return &img_71_snow;
        case 80: return is_day ? &img_61_rain_light_day : &img_61_rain_light_night;
        case 81: return &img_63_rain_medium;
        case 82: return &img_65_rain_heavy;
        case 85: return &img_71_snow;
        case 86: return &img_71_snow;
        case 95: return &img_95_thunderstorm;
        case 96: return &img_96_thunderstorm_hail;
        case 99: return &img_96_thunderstorm_hail;
        default: return &img_unknown;
    }
}

/* UI elemek */
static lv_obj_t* s_scr_page3 = NULL;
static lv_obj_t* card_weather = NULL;
static lv_obj_t* lbl_city = NULL;
static lv_obj_t* line_city_separator = NULL;
static lv_obj_t* img_weather = NULL;
static lv_obj_t* lbl_temp = NULL;
static lv_obj_t* lbl_hum = NULL;
static lv_obj_t* lbl_wind = NULL;
static lv_obj_t* lbl_code = NULL;
static lv_obj_t* lbl_uv = NULL;

static lv_timer_t* weather_timer = NULL;

/* Képernyő törlés callback */
static void screen_page3_delete_cb(lv_event_t* e) {
    if (weather_timer) {
        lv_timer_del(weather_timer);
        weather_timer = NULL;
    }
    s_scr_page3 = NULL;
}

/* Teszt / Demo frissítő (a valós Wi-Fi bekötésig) */
static void update_weather_cb(lv_timer_t* t) {
    if (lv_screen_active() != s_scr_page3) return;

    /* Teszt adatok: Napos idő, 23.4 °C */
    bool day = true;
    int code = 1; // Mostly Clear

    // Kártya színe nappal/éjjel
    if (day) {
        lv_obj_set_style_bg_color(card_weather, lv_color_hex(0x87CEFA), 0);
        lv_obj_set_style_bg_grad_color(card_weather, lv_color_hex(0x37C4E8), 0);
    } else {
        lv_obj_set_style_bg_color(card_weather, lv_color_hex(0x001F72), 0);
        lv_obj_set_style_bg_grad_color(card_weather, lv_color_hex(0x001F3F), 0);
    }
    lv_obj_set_style_bg_grad_dir(card_weather, LV_GRAD_DIR_VER, 0);

    lv_color_t text_color = day ? lv_color_hex(0x2B3990) : lv_color_white();
    lv_obj_set_style_text_color(lbl_city, text_color, 0);
    lv_obj_set_style_text_color(lbl_temp, text_color, 0);
    lv_obj_set_style_text_color(lbl_hum, text_color, 0);
    lv_obj_set_style_text_color(lbl_wind, text_color, 0);
    lv_obj_set_style_text_color(lbl_code, text_color, 0);
    lv_obj_set_style_text_color(lbl_uv, text_color, 0);
    lv_obj_set_style_bg_color(line_city_separator, text_color, 0);

    lv_image_set_src(img_weather, weathercode_to_icon(code, day));
    lv_label_set_text(lbl_city, "Budapest");
    lv_label_set_text(lbl_temp, "23.4°C");
    lv_label_set_text(lbl_code, weathercode_to_text(code));
    lv_label_set_text(lbl_hum, "Humidity: 48 %");
    lv_label_set_text(lbl_wind, "Wind: 3.2 m/s  140°");
    lv_label_set_text(lbl_uv, "UV index: 5.1");
}

/* ---- Képernyő létrehozása 480x320 felbontásra ---- */
lv_obj_t* screen_page3_create(void) {
    lv_obj_t* scr = lv_obj_create(NULL);
    s_scr_page3 = scr;

    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(scr, screen_page3_delete_cb, LV_EVENT_DELETE, NULL);

    /* ---- 1. Főkártya (460 x 300 px) ---- */
    card_weather = lv_obj_create(scr);
    lv_obj_set_size(card_weather, 460, 300);
    lv_obj_align(card_weather, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(card_weather, 12, 0);
    lv_obj_set_style_border_width(card_weather, 0, 0);
    lv_obj_set_style_pad_all(card_weather, 0, 0);
    lv_obj_set_style_bg_opa(card_weather, LV_OPA_COVER, 0); // <--- EZ HIÁNYZOTT A HÁTTÉRSZÍNHEZ!
    lv_obj_clear_flag(card_weather, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- 2. Függőleges elválasztó vonal középen ---- */
    line_city_separator = lv_obj_create(card_weather);
    lv_obj_clear_flag(line_city_separator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(line_city_separator, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(line_city_separator, 3, 240);
    lv_obj_align(line_city_separator, LV_ALIGN_CENTER, -10, 0);
    lv_obj_set_style_pad_all(line_city_separator, 0, 0);
    lv_obj_set_style_border_width(line_city_separator, 0, 0);
    lv_obj_set_style_bg_opa(line_city_separator, LV_OPA_COVER, 0); // <--- KÖZVETLEN FEDETTSÉG
    lv_obj_set_style_bg_color(line_city_separator, lv_color_hex(0x2B3990), 0); // Kezdeti nappali szín

    /* ---- BAL OLDAL: Állapot felirat és WMO Ikon ---- */
    lbl_code = lv_label_create(card_weather);
    lv_obj_set_style_text_font(lbl_code, &lv_font_montserrat_24, 0);
    lv_label_set_text(lbl_code, "Mostly Clear");
    lv_obj_align(lbl_code, LV_ALIGN_TOP_LEFT, 25, 30);

    img_weather = lv_image_create(card_weather);
    lv_image_set_src(img_weather, &img_01_mainly_clear_day);
    lv_image_set_scale(img_weather, 280); // Arányos WMO ikonméret
    lv_obj_align(img_weather, LV_ALIGN_LEFT_MID, 45, 25);

    /* ---- JOBB OLDAL: Város, Hőmérséklet és részletek ---- */
    lbl_city = lv_label_create(card_weather);
    lv_obj_set_style_text_font(lbl_city, &lv_font_montserrat_28, 0);
    lv_label_set_text(lbl_city, "Budapest");
    lv_obj_align(lbl_city, LV_ALIGN_TOP_LEFT, 250, 30);

    lbl_temp = lv_label_create(card_weather);
    lv_obj_set_style_text_font(lbl_temp, &lv_montserrat_60, 0); // Kiemelt méret
    lv_label_set_text(lbl_temp, "23.4°C");
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_LEFT, 250, 100);

    lbl_hum = lv_label_create(card_weather);
    lv_obj_set_style_text_font(lbl_hum, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_hum, "Humidity: 48 %");
    lv_obj_align(lbl_hum, LV_ALIGN_TOP_LEFT, 250, 180);

    lbl_wind = lv_label_create(card_weather);
    lv_obj_set_style_text_font(lbl_wind, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_wind, "Wind: 3.2 m/s  140°");
    lv_obj_align(lbl_wind, LV_ALIGN_TOP_LEFT, 250, 215);

    lbl_uv = lv_label_create(card_weather);
    lv_obj_set_style_text_font(lbl_uv, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_uv, "UV index: 5.1");
    lv_obj_align(lbl_uv, LV_ALIGN_TOP_LEFT, 250, 250);

    /* Kezdeti színezés és állapot betöltése */
    update_weather_cb(NULL);

    return scr;
}