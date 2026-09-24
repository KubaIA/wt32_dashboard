#include "screen_page5.h"
#include "ui_common.h"
#include "app.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gree_service.h"
#include "config.h"

/* Képek */
#include "pictures/img_off.h"
#include "pictures/img_hot.h"
#include "pictures/img_frost.h"

#define AC_COUNT 3

/* Geometria 480x320-ra: 3 kártya fixen */
#define CARD_W       148
#define CARD_H       296
#define CARD_SPACING 9
#define CARD_TOP_Y   12

static lv_obj_t* s_scr_page5 = NULL;
static lv_timer_t* s_page5_timer = NULL;

/* Stílusok a kártyákhoz */
static lv_style_t style_card_idle;
static lv_style_t style_card_cool;
static lv_style_t style_card_heat;
static bool styles_inited = false;

typedef struct {
    lv_obj_t* card;
    lv_obj_t* ip_lbl;
    lv_obj_t* power_sw;
    lv_obj_t* temp_slider;
    lv_obj_t* temp_val_lbl;
    lv_obj_t* icon_obj;
    lv_obj_t* status_lbl;
} ac_ui_t;

static ac_ui_t s_ac[AC_COUNT];

static void init_custom_styles(void) {
    if (styles_inited) return;

    lv_style_init(&style_card_idle);
    lv_style_set_radius(&style_card_idle, 10);
    lv_style_set_bg_opa(&style_card_idle, LV_OPA_COVER);
    lv_style_set_bg_color(&style_card_idle, lv_color_hex(0x383838));
    lv_style_set_border_width(&style_card_idle, 0);
    lv_style_set_text_color(&style_card_idle, lv_color_white());

    lv_style_init(&style_card_cool);
    lv_style_copy(&style_card_cool, &style_card_idle);
    lv_style_set_bg_color(&style_card_cool, lv_color_hex(0x1F4E79));

    lv_style_init(&style_card_heat);
    lv_style_copy(&style_card_heat, &style_card_idle);
    lv_style_set_bg_color(&style_card_heat, lv_color_hex(0x8B2500));

    styles_inited = true;
}

/* Állapotfrissítő segédfüggvény */
static void apply_ac_state(int idx, bool power, int set_temp, int room_temp) {
    if (idx < 0 || idx >= AC_COUNT) return;

    lv_obj_remove_style(s_ac[idx].card, &style_card_cool, LV_PART_MAIN);
    lv_obj_remove_style(s_ac[idx].card, &style_card_heat, LV_PART_MAIN);
    lv_obj_remove_style(s_ac[idx].card, &style_card_idle, LV_PART_MAIN);

    if (!power) {
        // Kikapcsolt állapot: szürke kártya, letiltott slider
        lv_obj_add_style(s_ac[idx].card, &style_card_idle, LV_PART_MAIN);
        lv_image_set_src(s_ac[idx].icon_obj, &img_off);
        
        // Slider letiltása, hogy ne lehessen elhúzni
        lv_obj_add_state(s_ac[idx].temp_slider, LV_STATE_DISABLED);

        // Kikapcsolva a csúszka és a felirat a szobahőmérsékletet mutatja
        int disp_temp = room_temp;
        if (disp_temp < 18) disp_temp = 18;
        if (disp_temp > 30) disp_temp = 30;

        lv_slider_set_value(s_ac[idx].temp_slider, disp_temp, LV_ANIM_OFF);
        lv_label_set_text_fmt(s_ac[idx].temp_val_lbl, "%d°C", room_temp);
    } else {
        // Bekapcsolt állapot: slider aktív
        lv_obj_clear_state(s_ac[idx].temp_slider, LV_STATE_DISABLED);

        if (room_temp > set_temp) {
            lv_obj_add_style(s_ac[idx].card, &style_card_cool, LV_PART_MAIN);
            lv_image_set_src(s_ac[idx].icon_obj, &img_frost);
        } else if (room_temp < set_temp) {
            lv_obj_add_style(s_ac[idx].card, &style_card_heat, LV_PART_MAIN);
            lv_image_set_src(s_ac[idx].icon_obj, &img_hot);
        } else {
            lv_obj_add_style(s_ac[idx].card, &style_card_idle, LV_PART_MAIN);
            lv_image_set_src(s_ac[idx].icon_obj, &img_off);
        }

        // Bekapcsolva a célhőmérsékletet mutatja a slider és a felirat
        lv_slider_set_value(s_ac[idx].temp_slider, set_temp, LV_ANIM_OFF);
        lv_label_set_text_fmt(s_ac[idx].temp_val_lbl, "%d°C", set_temp);
    }

    lv_label_set_text_fmt(s_ac[idx].status_lbl, "Room: %d°C", room_temp);
}

/* Periodikus frissítő: másodpercenként kiolvassa a valós telemetriát */
static void page5_update_timer_cb(lv_timer_t* timer) {
    if (!s_scr_page5) return;

    for (int i = 0; i < AC_COUNT; i++) {
        gree_state_t st;
        if (gree_service_get_state(i, &st)) {
            // Switch állapot szinkronizálása a valós adatokkal
            if (st.power) {
                lv_obj_add_state(s_ac[i].power_sw, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(s_ac[i].power_sw, LV_STATE_CHECKED);
            }

            apply_ac_state(i, st.power, st.temp_set, st.temp_current);
        } else {
            lv_label_set_text(s_ac[i].status_lbl, "Connecting...");
        }
    }
}

/* Callbackek az érintéshez - valós parancsküldéssel */
static void power_sw_cb(lv_event_t* e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t* sw = lv_event_get_target_obj(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    // Parancs küldése
    gree_service_set_power(idx, on);

    // Helyi felület azonnali frissítése a memóriában lévő adatokkal
    gree_state_t st;
    int room = 24, set_temp = 24;
    if (gree_service_get_state(idx, &st)) {
        room = st.temp_current;
        set_temp = st.temp_set;
    }
    apply_ac_state(idx, on, set_temp, room);
}

static void temp_slider_cb(lv_event_t* e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t* slider = lv_event_get_target_obj(e);
    int val = lv_slider_get_value(slider);

    // Hőmérséklet parancs kiküldése
    gree_service_set_temp(idx, (int8_t)val);

    // Helyi felület azonnali reagálása
    bool on = lv_obj_has_state(s_ac[idx].power_sw, LV_STATE_CHECKED);
    gree_state_t st;
    int room = 24;
    if (gree_service_get_state(idx, &st)) room = st.temp_current;
    apply_ac_state(idx, on, val, room);
}

static void screen_page5_delete_cb(lv_event_t* e) {
    if (s_page5_timer) {
        lv_timer_del(s_page5_timer);
        s_page5_timer = NULL;
    }
    s_scr_page5 = NULL;
}

lv_obj_t* screen_page5_create(void) {
    init_custom_styles();
    memset(s_ac, 0, sizeof(s_ac));

    lv_obj_t* scr = lv_obj_create(NULL);
    s_scr_page5 = scr;
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(scr, screen_page5_delete_cb, LV_EVENT_DELETE, NULL);

    for (int i = 0; i < AC_COUNT; i++) {
        int x_pos = CARD_SPACING + i * (CARD_W + CARD_SPACING);

        /* Kártya konténer */
        lv_obj_t* card = lv_obj_create(scr);
        s_ac[i].card = card;
        lv_obj_set_size(card, CARD_W, CARD_H);
        lv_obj_set_pos(card, x_pos, CARD_TOP_Y);
        lv_obj_add_style(card, &style_card_idle, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /* 1. IP Cím a konfigurációból */
        s_ac[i].ip_lbl = lv_label_create(card);
        const char* dev_ip = g_cfg.gree.dev[i].ip;
        if (dev_ip && strlen(dev_ip) > 0) {
            lv_label_set_text(s_ac[i].ip_lbl, dev_ip);
        } else {
            lv_label_set_text_fmt(s_ac[i].ip_lbl, "AC #%d", i + 1);
        }
        lv_obj_set_style_text_font(s_ac[i].ip_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(s_ac[i].ip_lbl, LV_ALIGN_TOP_MID, 0, 8);

        /* 2. Power felirat + Switch */
        lv_obj_t* power_lbl = lv_label_create(card);
        lv_label_set_text(power_lbl, "Power");
        lv_obj_set_style_text_font(power_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(power_lbl, LV_ALIGN_TOP_LEFT, 16, 40);

        s_ac[i].power_sw = lv_switch_create(card);
        lv_obj_set_size(s_ac[i].power_sw, 48, 24);
        lv_obj_align(s_ac[i].power_sw, LV_ALIGN_TOP_RIGHT, -16, 34);
        lv_obj_add_event_cb(s_ac[i].power_sw, power_sw_cb, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
        lv_obj_set_style_bg_color(s_ac[i].power_sw, lv_color_hex(0x4CAF50), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(s_ac[i].power_sw, lv_color_white(), LV_PART_KNOB);

        /* 3. Temperature felirat */
        lv_obj_t* temp_lbl = lv_label_create(card);
        lv_label_set_text(temp_lbl, "Temperature:");
        lv_obj_set_style_text_font(temp_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(temp_lbl, LV_ALIGN_TOP_MID, 0, 74);

        /* 4. Slider */
        s_ac[i].temp_slider = lv_slider_create(card);
        lv_obj_set_size(s_ac[i].temp_slider, 124, 14);
        lv_obj_align(s_ac[i].temp_slider, LV_ALIGN_TOP_MID, 0, 102);
        lv_slider_set_range(s_ac[i].temp_slider, 18, 30);
        lv_slider_set_value(s_ac[i].temp_slider, 22, LV_ANIM_OFF);
        lv_obj_add_event_cb(s_ac[i].temp_slider, temp_slider_cb, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
        lv_obj_set_style_bg_color(s_ac[i].temp_slider, lv_color_hex(0xCCCCCC), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(s_ac[i].temp_slider, lv_color_white(), LV_PART_KNOB);
        lv_obj_set_style_pad_all(s_ac[i].temp_slider, 2, LV_PART_KNOB);

        /* 5. Hőmérséklet érték */
        s_ac[i].temp_val_lbl = lv_label_create(card);
        lv_label_set_text(s_ac[i].temp_val_lbl, "22°C");
        lv_obj_set_style_text_font(s_ac[i].temp_val_lbl, &lv_font_montserrat_20, 0);
        lv_obj_align(s_ac[i].temp_val_lbl, LV_ALIGN_TOP_MID, 0, 126);

        /* 6. Ikon */
        s_ac[i].icon_obj = lv_image_create(card);
        lv_image_set_src(s_ac[i].icon_obj, &img_off);
        lv_image_set_scale(s_ac[i].icon_obj, 160);
        lv_obj_set_style_image_recolor(s_ac[i].icon_obj, lv_color_white(), 0);
        lv_obj_set_style_image_recolor_opa(s_ac[i].icon_obj, LV_OPA_COVER, 0);
        lv_obj_set_style_image_recolor(s_ac[i].icon_obj, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_align(s_ac[i].icon_obj, LV_ALIGN_TOP_MID, 0, 150);

        /* 7. Státusz (Room temp) */
        s_ac[i].status_lbl = lv_label_create(card);
        lv_label_set_text(s_ac[i].status_lbl, "Room: --");
        lv_obj_set_style_text_font(s_ac[i].status_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(s_ac[i].status_lbl, LV_ALIGN_BOTTOM_MID, 0, -8);
    }

    // Első azonnali frissítés, majd 1000 ms-os periodikus frissítés
    page5_update_timer_cb(NULL);
    s_page5_timer = lv_timer_create(page5_update_timer_cb, 1000, NULL);

    return scr;
}