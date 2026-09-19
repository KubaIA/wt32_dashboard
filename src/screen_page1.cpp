#include "screen_page1.h"
#include "ui_common.h"
#include "fonts/quartz_35.h"
#include "fonts/quartz_50.h"
#include "fonts/quartz_75.h"
#include "fonts/quartz_110.h"
#include <time.h>
#include <stdio.h>

/* UI elemek pointerei */
static lv_obj_t* lbl_tz   = NULL;
static lv_obj_t* lbl_time = NULL;
static lv_obj_t* lbl_date = NULL;
static lv_obj_t* lbl_sec  = NULL;

static lv_timer_t* page1_timer = NULL;
static lv_obj_t* s_scr_page1   = NULL;

/* ---- Idő frissítő callback (egyelőre belső millis / RTC alapon) ---- */
static void page1_time_update_cb(lv_timer_t* t) {
    if (lv_screen_active() != s_scr_page1) return;

    time_t now_ts = time(NULL);
    struct tm* now = localtime(&now_ts);

    static const char* days[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

    if (lbl_date) {
        lv_label_set_text_fmt(lbl_date, "%04d.%02d.%02d %s",
            now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, days[now->tm_wday]);
    }
    if (lbl_time) {
        lv_label_set_text_fmt(lbl_time, "%02d:%02d", now->tm_hour, now->tm_min);
    }
    if (lbl_sec) {
        lv_label_set_text_fmt(lbl_sec, "%02d", now->tm_sec);
    }
}

/* ---- Képernyő törlésekor időzítő leállítása ---- */
static void screen_page1_delete_cb(lv_event_t* e) {
    if (page1_timer) {
        lv_timer_del(page1_timer);
        page1_timer = NULL;
    }
    s_scr_page1 = NULL;
    lbl_tz = NULL;
    lbl_time = NULL;
    lbl_date = NULL;
    lbl_sec = NULL;
}

/* ---- Page 1 Képernyő létrehozása (480x320 geometriára szabva) ---- */
lv_obj_t* screen_page1_create(void) {
    lv_obj_t* scr = lv_obj_create(NULL);
    s_scr_page1 = scr;

    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(scr, screen_page1_delete_cb, LV_EVENT_DELETE, NULL);

    /* ---- IDŐ + DÁTUM KÁRTYA (460 x 300 pixel, 10-10 px margóval) ---- */
    lv_obj_t* card = lv_obj_create(scr);
    lv_obj_set_size(card, 460, 300);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);

    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x849058), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- 1. DÁTUM (Fent középen, quartz_50) ---- */
    lbl_date = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_date, &quartz_50, 0);
    lv_label_set_text(lbl_date, "2026.04.14 TUE");
    lv_obj_align(lbl_date, LV_ALIGN_TOP_MID, 0, 20);

    /* ---- 2. IDŐ: ÓRA:PERC (Középen balra tolva, quartz_110) ---- */
    lbl_time = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_time, &quartz_110, 0);
    lv_label_set_text(lbl_time, "12:00");
    lv_obj_align(lbl_time, LV_ALIGN_TOP_LEFT, 15, 100);

    /* ---- 3. MÁSODPERC (Óra mellett jobbra, quartz_75) ---- */
    lbl_sec = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_sec, &quartz_75, 0);
    lv_label_set_text(lbl_sec, "00");
    lv_obj_align(lbl_sec, LV_ALIGN_TOP_RIGHT, -20, 125);

    /* ---- 4. IDŐZÓNA CÍMKE (Alul középen, quartz_35) ---- */
    lbl_tz = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_tz, &quartz_35, 0);
    lv_label_set_text(lbl_tz, "EUROPE/BUDAPEST");
    lv_obj_align(lbl_tz, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* Frissítő timer (1 másodperc) */
    page1_timer = lv_timer_create(page1_time_update_cb, 1000, NULL);
    page1_time_update_cb(NULL);

    return scr;
}