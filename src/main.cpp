#include <Arduino.h>
#include <lvgl.h>
#include "LGFX_WT32_SC01_PLUS.h"
#include "app.h"
#include "config.h"
#include "net_service.h"
#include "gree_service.h"

LGFX lcd;

#define DRAW_BUF_SIZE (480 * 32 * sizeof(lv_color16_t))
static uint8_t draw_buf[DRAW_BUF_SIZE];

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((uint16_t *)px_map, w * h, true);
    lcd.endWrite();

    lv_display_flush_ready(disp);
}

void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
    int32_t x, y;
    bool touched = lcd.getTouch(&x, &y);

    if (touched) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void display_set_brightness(uint8_t b_percent) {
    if (b_percent < 10) b_percent = 10;
    if (b_percent > 100) b_percent = 100;
    // 10-100% átskálázása 25-255 közé (hogy sose kapcsoljon le teljesen feketére)
    uint8_t duty = (uint8_t)((b_percent * 255) / 100);
    lcd.setBrightness(duty);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    /* ---- Konfiguráció betöltése SD-kártyáról ---- */
    config_load(&g_cfg);
    /* ---- Hálózati szolgáltatás inicializálása ---- */
    net_service_init();
    /// ---- Gree szolgáltatás inicializálása ---- */
    gree_service_init();

    lcd.init();
    lcd.setRotation(1);
    display_set_brightness(g_cfg.display.brightness);

    lv_init();

    lv_display_t *disp = lv_display_create(480, 320);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read);

    /* ALKALMAZÁS INDÍTÁSA A LAPOZÓ MOTORRAL */
    app_init();
}

void loop() {
    static uint32_t last_tick = 0;
    uint32_t now = millis();
    lv_tick_inc(now - last_tick);
    last_tick = now;

    lv_timer_handler();
    delay(5);
}