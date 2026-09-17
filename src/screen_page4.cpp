#include "screen_page4.h"
#include "ui_common.h"
#include "app.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "pictures/img_solar.h"
#include "pictures/img_house.h"
#include "pictures/img_grid.h"
#include "pictures/img_battery.h"

/* --- Geometria 480x320 kijelzőhöz --- */
#define BAR_W    105
#define BAR_H    290
#define SPACING  12
#define TOP_Y    15

static lv_obj_t* s_scr_page4 = NULL;

/* --- Labelek --- */
static lv_obj_t* lbl_ip;
static lv_obj_t* lbl_pv_name,   * lbl_pv_value,   * lbl_pv_unit;
static lv_obj_t* lbl_soc_name,  * lbl_soc_value,  * lbl_soc_unit;
static lv_obj_t* lbl_load_name, * lbl_load_value, * lbl_load_unit;
static lv_obj_t* lbl_grid_name, * lbl_grid_value, * lbl_grid_unit;

/* --- Sávok --- */
static lv_obj_t* bar_pv_bg,   * bar_pv_fill;
static lv_obj_t* bar_soc;
static lv_obj_t* bar_load_bg, * bar_load_fill;
static lv_obj_t* bar_grid_bg, * bar_grid_fill;

/* --- Ikonok --- */
static lv_obj_t* icon_pv;
static lv_obj_t* icon_soc;
static lv_obj_t* icon_load;
static lv_obj_t* icon_grid;

/* --- PV bar frissítés --- */
static void update_pv_bar(int pv_w) {
    const int max_w = 6000;
    if (pv_w < 0) pv_w = 0;
    if (pv_w > max_w) pv_w = max_w;

    int32_t fill_h = (int32_t)(((double)pv_w / (double)max_w) * BAR_H);
    lv_obj_set_size(bar_pv_fill, BAR_W, fill_h);
    lv_obj_set_pos(bar_pv_fill, 0, BAR_H - fill_h);
    lv_obj_set_style_bg_color(bar_pv_fill, lv_palette_main(LV_PALETTE_GREEN), 0);
}

/* --- Load bar frissítés --- */
static void update_load_bar(int load_w) {
    const int max_w = 12000;
    if (load_w < 0) load_w = 0;
    if (load_w > max_w) load_w = max_w;

    int32_t fill_h = (int32_t)(((double)load_w / (double)max_w) * BAR_H);
    lv_obj_set_size(bar_load_fill, BAR_W, fill_h);
    lv_obj_set_pos(bar_load_fill, 0, BAR_H - fill_h);
    lv_obj_set_style_bg_color(bar_load_fill, lv_palette_main(LV_PALETTE_RED), 0);
}

/* --- Grid bar frissítés (Zöld export, Piros import) --- */
static void update_grid_bar(int grid_w) {
    const int export_max_w = 6000;
    const int import_max_w = 12000;

    if (grid_w < 0) {
        int export_w = -grid_w;
        if (export_w > export_max_w) export_w = export_max_w;

        int32_t fill_h = (int32_t)(((double)export_w / (double)export_max_w) * BAR_H);
        lv_obj_set_pos(bar_grid_fill, 0, BAR_H - fill_h);
        lv_obj_set_size(bar_grid_fill, BAR_W, fill_h);
        lv_obj_set_style_bg_color(bar_grid_fill, lv_palette_main(LV_PALETTE_GREEN), 0);
    } else {
        int import_w = grid_w;
        if (import_w > import_max_w) import_w = import_max_w;

        int32_t fill_h = (int32_t)(((double)import_w / (double)import_max_w) * BAR_H);
        lv_obj_set_pos(bar_grid_fill, 0, BAR_H - fill_h);
        lv_obj_set_size(bar_grid_fill, BAR_W, fill_h);
        lv_obj_set_style_bg_color(bar_grid_fill, lv_palette_main(LV_PALETTE_RED), 0);
    }
}

static void screen_page4_delete_cb(lv_event_t* e) {
    s_scr_page4 = NULL;
}

lv_obj_t* screen_page4_create(void) {
    lv_obj_t* scr = lv_obj_create(NULL);
    s_scr_page4 = scr;
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(scr, screen_page4_delete_cb, LV_EVENT_DELETE, NULL);

    int32_t x_pv   = SPACING;
    int32_t x_soc  = x_pv + BAR_W + SPACING;
    int32_t x_load = x_soc + BAR_W + SPACING;
    int32_t x_grid = x_load + BAR_W + SPACING;

    /* ---- 1. PV (Solar) Bar ---- */
    bar_pv_bg = lv_obj_create(scr);
    lv_obj_set_size(bar_pv_bg, BAR_W, BAR_H);
    lv_obj_set_pos(bar_pv_bg, x_pv, TOP_Y);
    lv_obj_set_style_bg_color(bar_pv_bg, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(bar_pv_bg, LV_OPA_30, 0);
    lv_obj_set_style_border_width(bar_pv_bg, 0, 0);
    lv_obj_set_style_radius(bar_pv_bg, 8, 0);
    lv_obj_set_style_pad_all(bar_pv_bg, 0, 0);
    lv_obj_clear_flag(bar_pv_bg, LV_OBJ_FLAG_SCROLLABLE);

    bar_pv_fill = lv_obj_create(bar_pv_bg);
    lv_obj_set_style_border_width(bar_pv_fill, 0, 0);
    lv_obj_set_style_radius(bar_pv_fill, 8, 0);
    lv_obj_clear_flag(bar_pv_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bar_pv_fill, lv_palette_main(LV_PALETTE_GREEN), 0);

    /* ---- 2. Battery (SOC) Bar (LVGL natív bar) ---- */
    bar_soc = lv_bar_create(scr);
    lv_obj_set_size(bar_soc, BAR_W, BAR_H);
    lv_obj_set_pos(bar_soc, x_soc, TOP_Y);
    lv_obj_set_style_bg_color(bar_soc, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar_soc, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_soc, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_soc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_soc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);

    /* ---- 3. Load (House) Bar ---- */
    bar_load_bg = lv_obj_create(scr);
    lv_obj_set_size(bar_load_bg, BAR_W, BAR_H);
    lv_obj_set_pos(bar_load_bg, x_load, TOP_Y);
    lv_obj_set_style_bg_color(bar_load_bg, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(bar_load_bg, LV_OPA_30, 0);
    lv_obj_set_style_border_width(bar_load_bg, 0, 0);
    lv_obj_set_style_radius(bar_load_bg, 8, 0);
    lv_obj_set_style_pad_all(bar_load_bg, 0, 0);
    lv_obj_clear_flag(bar_load_bg, LV_OBJ_FLAG_SCROLLABLE);

    bar_load_fill = lv_obj_create(bar_load_bg);
    lv_obj_set_style_border_width(bar_load_fill, 0, 0);
    lv_obj_set_style_radius(bar_load_fill, 8, 0);
    lv_obj_clear_flag(bar_load_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bar_load_fill, lv_palette_main(LV_PALETTE_RED), 0);

    /* ---- 4. Grid Bar ---- */
    bar_grid_bg = lv_obj_create(scr);
    lv_obj_set_size(bar_grid_bg, BAR_W, BAR_H);
    lv_obj_set_pos(bar_grid_bg, x_grid, TOP_Y);
    lv_obj_set_style_bg_color(bar_grid_bg, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(bar_grid_bg, LV_OPA_30, 0);
    lv_obj_set_style_border_width(bar_grid_bg, 0, 0);
    lv_obj_set_style_radius(bar_grid_bg, 8, 0);
    lv_obj_set_style_pad_all(bar_grid_bg, 0, 0);
    lv_obj_clear_flag(bar_grid_bg, LV_OBJ_FLAG_SCROLLABLE);

    bar_grid_fill = lv_obj_create(bar_grid_bg);
    lv_obj_set_style_border_width(bar_grid_fill, 0, 0);
    lv_obj_set_style_radius(bar_grid_fill, 8, 0);
    lv_obj_clear_flag(bar_grid_fill, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- INVERTER IP CÍM (A 3. oszlop, House felett az eredeti logika szerint) ---- */
    lbl_ip = lv_label_create(scr);
    lv_label_set_text(lbl_ip, g_cfg.inverter.ip);
    lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_ip, lv_color_white(), 0);
    
    // A harmadik oszlop (bar_load_bg) felső részéhez igazítjuk:
    lv_obj_align_to(lbl_ip, bar_load_bg, LV_ALIGN_TOP_MID, 0, 6);
    /* =============================================================== */

    /* ---- IKONOK (Újraszínezve fehérre és skálázva) ---- */
    icon_pv = lv_image_create(scr);
    lv_image_set_src(icon_pv, &img_solar);
    lv_image_set_scale(icon_pv, 170);
    lv_obj_set_style_image_recolor(icon_pv, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_pv, LV_OPA_COVER, 0);

    icon_soc = lv_image_create(scr);
    lv_image_set_src(icon_soc, &img_battery);
    lv_image_set_scale(icon_soc, 170);
    lv_obj_set_style_image_recolor(icon_soc, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_soc, LV_OPA_COVER, 0);

    icon_load = lv_image_create(scr);
    lv_image_set_src(icon_load, &img_house);
    lv_image_set_scale(icon_load, 170);
    lv_obj_set_style_image_recolor(icon_load, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_load, LV_OPA_COVER, 0);

    icon_grid = lv_image_create(scr);
    lv_image_set_src(icon_grid, &img_grid);
    lv_image_set_scale(icon_grid, 170);
    lv_obj_set_style_image_recolor(icon_grid, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_grid, LV_OPA_COVER, 0);

    /* ---- LABELEK ---- */
    lbl_pv_name   = lv_label_create(scr); lbl_pv_value   = lv_label_create(scr); lbl_pv_unit   = lv_label_create(scr);
    lbl_soc_name  = lv_label_create(scr); lbl_soc_value  = lv_label_create(scr); lbl_soc_unit  = lv_label_create(scr);
    lbl_load_name = lv_label_create(scr); lbl_load_value = lv_label_create(scr); lbl_load_unit = lv_label_create(scr);
    lbl_grid_name = lv_label_create(scr); lbl_grid_value = lv_label_create(scr); lbl_grid_unit = lv_label_create(scr);

    /* Stílusok és szövegek beállítása */
    lv_obj_t* names[] = { lbl_pv_name, lbl_soc_name, lbl_load_name, lbl_grid_name };
    const char* name_texts[] = { "Solar", "Battery", "House", "Grid" };
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_text_font(names[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(names[i], lv_color_white(), 0);
        lv_label_set_text(names[i], name_texts[i]);
    }

    lv_obj_t* values[] = { lbl_pv_value, lbl_soc_value, lbl_load_value, lbl_grid_value };
    const char* val_texts[] = { "3.45", "85", "1.20", "2.25" };
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_text_font(values[i], &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(values[i], lv_color_white(), 0);
        lv_obj_set_width(values[i], 100);
        lv_obj_set_style_text_align(values[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(values[i], val_texts[i]);
    }

    lv_obj_t* units[] = { lbl_pv_unit, lbl_soc_unit, lbl_load_unit, lbl_grid_unit };
    const char* unit_texts[] = { "kW", "%", "kW", "kW" };
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_text_font(units[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(units[i], lv_color_white(), 0);
        lv_label_set_text(units[i], unit_texts[i]);
    }

    /* ---- ELHELYEZÉS (480x320-ra optimalizált Y koordináták) ---- */
    const int32_t y_center_shift = 50; // Középpont eltolása lefelé az alsó szövegcsoporthoz

    /* Ikonok igazítása (fent az oszlopok közepén) */
    lv_obj_align_to(icon_pv,   bar_pv_bg,   LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_align_to(icon_soc,  bar_soc,     LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_align_to(icon_load, bar_load_bg, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_align_to(icon_grid, bar_grid_bg, LV_ALIGN_TOP_MID, 0, 45);

    /* PV Labelek */
    lv_obj_align_to(lbl_pv_name,  bar_pv_bg, LV_ALIGN_CENTER, 0,  -5 + y_center_shift);
    lv_obj_align_to(lbl_pv_value, bar_pv_bg, LV_ALIGN_CENTER, 0,  20 + y_center_shift);
    lv_obj_align_to(lbl_pv_unit,  bar_pv_bg, LV_ALIGN_CENTER, 0,  45 + y_center_shift);

    /* Battery Labelek */
    lv_obj_align_to(lbl_soc_name,  bar_soc, LV_ALIGN_CENTER, 0,  -5 + y_center_shift);
    lv_obj_align_to(lbl_soc_value, bar_soc, LV_ALIGN_CENTER, 0,  20 + y_center_shift);
    lv_obj_align_to(lbl_soc_unit,  bar_soc, LV_ALIGN_CENTER, 0,  45 + y_center_shift);

    /* House Labelek */
    lv_obj_align_to(lbl_load_name,  bar_load_bg, LV_ALIGN_CENTER, 0,  -5 + y_center_shift);
    lv_obj_align_to(lbl_load_value, bar_load_bg, LV_ALIGN_CENTER, 0,  20 + y_center_shift);
    lv_obj_align_to(lbl_load_unit,  bar_load_bg, LV_ALIGN_CENTER, 0,  45 + y_center_shift);

    /* Grid Labelek */
    lv_obj_align_to(lbl_grid_name,  bar_grid_bg, LV_ALIGN_CENTER, 0,  -5 + y_center_shift);
    lv_obj_align_to(lbl_grid_value, bar_grid_bg, LV_ALIGN_CENTER, 0,  20 + y_center_shift);
    lv_obj_align_to(lbl_grid_unit,  bar_grid_bg, LV_ALIGN_CENTER, 0,  45 + y_center_shift);

    /* Kezdeti szintek teszthez (3.45 kW solar, 85% akku, 1.2 kW ház, -2.25 kW visszatáplálás) */
    update_pv_bar(3450);
    lv_bar_set_value(bar_soc, 85, LV_ANIM_OFF);
    update_load_bar(1200);
    update_grid_bar(-2250); // Negatív = Zöld export sáv

    return scr;
}

void screen_page4_force_update_ip(const char* ip) {
    if (lbl_ip && ip) {
        lv_label_set_text(lbl_ip, ip);
    }
}