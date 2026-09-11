#include "screen_page2.h"
#include "ui_common.h"
#include <stdio.h>
#include <math.h>

#include "pictures/img_cpu.h"
#include "pictures/img_ram.h"
#include "pictures/img_hdd.h"
#include "pictures/img_net.h"

/* 480x320 kijelzőre átskálázott méretek */
#define BAR_H 290
#define BAR_W 105
#define GAP_X 12
#define START_Y 15

static lv_obj_t* s_scr_page2 = NULL;

/* --- Labelek --- */
static lv_obj_t* pc_name_tag, * pc_name_val;
static lv_obj_t* cpu_type, * cpu_val, * cpu_pct;
static lv_obj_t* ram_size, * ram_val, * ram_pct;
static lv_obj_t* disk_drive, * disk_val;
static lv_obj_t* disk_io_tag, * disk_io_val, * disk_io_unit;
static lv_obj_t* net_io_tag, * net_io_val, * net_io_unit;

/* --- Ikonok --- */
static lv_obj_t* icon_cpu;
static lv_obj_t* icon_ram;
static lv_obj_t* icon_disk;
static lv_obj_t* icon_net;

static lv_obj_t* f_cpu, * f_ram, * f_disk, * f_net;

/* Dinamikus kitöltősáv magasságát állító segédfüggvény */
void page2_update_fill(lv_obj_t* obj, double current, double max) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    if (current < 0) current = 0;
    if (current > max) current = max;
    int32_t fill_h = (int32_t)((current / max) * BAR_H);
    lv_obj_set_size(obj, lv_obj_get_width(obj), fill_h);
    lv_obj_set_pos(obj, lv_obj_get_x(obj), BAR_H - fill_h);
}

static void screen_page2_delete_cb(lv_event_t* e) {
    s_scr_page2 = NULL;
}

static lv_obj_t* create_bar_group(lv_obj_t* scr, int x, lv_color_t c1, lv_obj_t** f1) {
    lv_obj_t* bg = lv_obj_create(scr);
    lv_obj_set_size(bg, BAR_W, BAR_H);
    lv_obj_set_pos(bg, x, START_Y);
    lv_obj_set_style_bg_color(bg, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_30, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 8, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);

    *f1 = lv_obj_create(bg);
    lv_obj_set_size(*f1, BAR_W, 0);
    lv_obj_set_pos(*f1, 0, BAR_H);
    lv_obj_set_style_bg_color(*f1, c1, 0);
    lv_obj_set_style_radius(*f1, 8, 0);
    lv_obj_set_style_border_width(*f1, 0, 0);
    lv_obj_clear_flag(*f1, LV_OBJ_FLAG_SCROLLABLE);

    return bg;
}

lv_obj_t* screen_page2_create(void) {
    s_scr_page2 = lv_obj_create(NULL);
    lv_obj_clear_flag(s_scr_page2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr_page2, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(s_scr_page2, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(s_scr_page2, screen_page2_delete_cb, LV_EVENT_DELETE, NULL);

    int step = BAR_W + GAP_X; // 105 + 12 = 117 px lépésköz

    lv_obj_t* cpu_bar  = create_bar_group(s_scr_page2, GAP_X,            lv_palette_main(LV_PALETTE_GREEN),  &f_cpu);
    lv_obj_t* ram_bar  = create_bar_group(s_scr_page2, GAP_X + step,     lv_palette_main(LV_PALETTE_BLUE),   &f_ram);
    lv_obj_t* disk_bar = create_bar_group(s_scr_page2, GAP_X + step * 2, lv_palette_main(LV_PALETTE_ORANGE), &f_disk);
    lv_obj_t* net_bar  = create_bar_group(s_scr_page2, GAP_X + step * 3, lv_palette_main(LV_PALETTE_CYAN),   &f_net);

    /* --- IKONOK (Fehér újraszínezéssel és ~65%-os skálázással) --- */
    icon_cpu = lv_image_create(cpu_bar);
    lv_image_set_src(icon_cpu, &img_cpu);
    lv_image_set_scale(icon_cpu, 170); // Arányos méret a 105px oszlophoz
    lv_obj_set_style_image_recolor(icon_cpu, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_cpu, LV_OPA_COVER, 0);
    lv_obj_align(icon_cpu, LV_ALIGN_TOP_MID, 0, 58);

    icon_ram = lv_image_create(ram_bar);
    lv_image_set_src(icon_ram, &img_ram);
    lv_image_set_scale(icon_ram, 170);
    lv_obj_set_style_image_recolor(icon_ram, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_ram, LV_OPA_COVER, 0);
    lv_obj_align(icon_ram, LV_ALIGN_TOP_MID, 0, 58);

    icon_disk = lv_image_create(disk_bar);
    lv_image_set_src(icon_disk, &img_hdd); // img_hdd az eredeti kód szerint!
    lv_image_set_scale(icon_disk, 170);
    lv_obj_set_style_image_recolor(icon_disk, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_disk, LV_OPA_COVER, 0);
    lv_obj_align(icon_disk, LV_ALIGN_TOP_MID, 0, 58);

    icon_net = lv_image_create(net_bar);
    lv_image_set_src(icon_net, &img_net);
    lv_image_set_scale(icon_net, 170);
    lv_obj_set_style_image_recolor(icon_net, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon_net, LV_OPA_COVER, 0);
    lv_obj_align(icon_net, LV_ALIGN_TOP_MID, 0, 58);

    /* --- FELSŐ CÍMKÉK (Fejléc) --- */
    pc_name_tag = lv_label_create(cpu_bar);
    lv_label_set_text(pc_name_tag, "PC Name:");
    lv_obj_set_style_text_font(pc_name_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(pc_name_tag, LV_ALIGN_TOP_MID, 0, 8);

    pc_name_val = lv_label_create(cpu_bar);
    lv_label_set_text(pc_name_val, "DESKTOP");
    lv_obj_set_style_text_font(pc_name_val, &lv_font_montserrat_14, 0);
    lv_obj_align(pc_name_val, LV_ALIGN_TOP_MID, 0, 26);

    disk_drive = lv_label_create(disk_bar);
    lv_label_set_text(disk_drive, "C: 930 GB");
    lv_obj_set_style_text_font(disk_drive, &lv_font_montserrat_14, 0);
    lv_obj_align(disk_drive, LV_ALIGN_TOP_MID, 0, 8);

    disk_val = lv_label_create(disk_bar);
    lv_label_set_text(disk_val, "86%");
    lv_obj_set_style_text_font(disk_drive, &lv_font_montserrat_14, 0);
    lv_obj_align(disk_val, LV_ALIGN_TOP_MID, 0, 26);

    /* --- ALSÓ ÉRTÉKCSOPORTOK (Az eredeti 3 soros elrendezés Montserrat 28/14-gyel) --- */
    /* CPU */
    cpu_type = lv_label_create(cpu_bar);
    lv_label_set_text(cpu_type, "i9-10900");
    lv_obj_set_style_text_font(cpu_type, &lv_font_montserrat_14, 0);
    lv_obj_align(cpu_type, LV_ALIGN_CENTER, 0, 45);

    cpu_val = lv_label_create(cpu_bar);
    lv_label_set_text(cpu_val, "15.2");
    lv_obj_set_style_text_font(cpu_val, &lv_font_montserrat_28, 0);
    lv_obj_align(cpu_val, LV_ALIGN_CENTER, 0, 70);

    cpu_pct = lv_label_create(cpu_bar);
    lv_label_set_text(cpu_pct, "%");
    lv_obj_set_style_text_font(cpu_pct, &lv_font_montserrat_18, 0);
    lv_obj_align(cpu_pct, LV_ALIGN_CENTER, 0, 95);

    /* RAM */
    ram_size = lv_label_create(ram_bar);
    lv_label_set_text(ram_size, "32.0 GB");
    lv_obj_set_style_text_font(ram_size, &lv_font_montserrat_14, 0);
    lv_obj_align(ram_size, LV_ALIGN_CENTER, 0, 45);

    ram_val = lv_label_create(ram_bar);
    lv_label_set_text(ram_val, "58");
    lv_obj_set_style_text_font(ram_val, &lv_font_montserrat_28, 0);
    lv_obj_align(ram_val, LV_ALIGN_CENTER, 0, 70);

    ram_pct = lv_label_create(ram_bar);
    lv_label_set_text(ram_pct, "%");
    lv_obj_set_style_text_font(ram_pct, &lv_font_montserrat_18, 0);
    lv_obj_align(ram_pct, LV_ALIGN_CENTER, 0, 95);

    /* DISK IO */
    disk_io_tag = lv_label_create(disk_bar);
    lv_label_set_text(disk_io_tag, "DISK IO:");
    lv_obj_set_style_text_font(disk_io_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(disk_io_tag, LV_ALIGN_CENTER, 0, 45);

    disk_io_val = lv_label_create(disk_bar);
    lv_label_set_text(disk_io_val, "062.1");
    lv_obj_set_style_text_font(disk_io_val, &lv_font_montserrat_28, 0);
    lv_obj_align(disk_io_val, LV_ALIGN_CENTER, 0, 70);

    disk_io_unit = lv_label_create(disk_bar);
    lv_label_set_text(disk_io_unit, "MB/s");
    lv_obj_set_style_text_font(disk_io_unit, &lv_font_montserrat_18, 0);
    lv_obj_align(disk_io_unit, LV_ALIGN_CENTER, 0, 95);

    /* NET IO */
    net_io_tag = lv_label_create(net_bar);
    lv_label_set_text(net_io_tag, "NET IO:");
    lv_obj_set_style_text_font(net_io_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(net_io_tag, LV_ALIGN_CENTER, 0, 45);

    net_io_val = lv_label_create(net_bar);
    lv_label_set_text(net_io_val, "105.5");
    lv_obj_set_style_text_font(net_io_val, &lv_font_montserrat_28, 0);
    lv_obj_align(net_io_val, LV_ALIGN_CENTER, 0, 70);

    net_io_unit = lv_label_create(net_bar);
    lv_label_set_text(net_io_unit, "MB/s");
    lv_obj_set_style_text_font(net_io_unit, &lv_font_montserrat_18, 0);
    lv_obj_align(net_io_unit, LV_ALIGN_CENTER, 0, 95);

    /* Szövegszínek fehérre állítása */
    lv_obj_t* all_labels[] = {
        pc_name_tag, pc_name_val, cpu_type, cpu_val, cpu_pct, ram_size, ram_val, ram_pct,
        disk_drive, disk_val, disk_io_tag, disk_io_val, disk_io_unit,
        net_io_tag, net_io_val, net_io_unit
    };
    for (int i = 0; i < 16; i++) {
        if (all_labels[i]) {
            lv_obj_set_style_text_color(all_labels[i], lv_color_white(), 0);
        }
    }

    /* Kezdeti kitöltési szintek beállítása a sávokhoz */
    page2_update_fill(f_cpu, 25.0, 100.0);
    page2_update_fill(f_ram, 58.0, 100.0);
    page2_update_fill(f_disk, 62.0, 100.0);
    page2_update_fill(f_net, 45.0, 100.0);

    return s_scr_page2;
}