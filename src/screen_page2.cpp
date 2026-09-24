#include "screen_page2.h"
#include "ui_common.h"
#include "pc_service.h"
#include <stdio.h>
#include <string.h>
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

/* Dinamikus sáv skálázási maximumok */
#define MAX_IO_KBS 500000.0    // SATA SSD (~500 MB/s)
#define MAX_NET_KBS 12500.0    // 100 Mbps hálózat (~12.5 MB/s)

static lv_obj_t* s_scr_page2 = NULL;
static lv_timer_t* s_page2_timer = NULL;

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

void pc_service_get_debug_str(char* buf, size_t max_len);

/* Emulátorból átemelt dinamikus mértékegység és értékválasztó segédfüggvény */
static void split_dynamic_unit(double kbs_val, char* out_val, char* out_unit) {
    const char* units[] = { "B/s", "KB/s", "MB/s", "GB/s" };
    int unit_index = 1;
    double val = kbs_val;

    if (val > 0 && val < 1.0) {
        val *= 1024.0;
        unit_index = 0;
    } else {
        while (val >= 1000.0 && unit_index < 3) {
            val /= 1024.0;
            unit_index++;
        }
    }
    snprintf(out_val, 16, "%.2f", val);
    strncpy(out_unit, units[unit_index], 8);
    out_unit[7] = '\0';
}

/* Dinamikus kitöltősáv magasságát állító segédfüggvény */
void page2_update_fill(lv_obj_t* obj, double current, double max) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    if (current < 0) current = 0;
    if (current > max) current = max;
    int32_t fill_h = (int32_t)((current / max) * BAR_H);
    lv_obj_set_size(obj, lv_obj_get_width(obj), fill_h);
    lv_obj_set_pos(obj, lv_obj_get_x(obj), BAR_H - fill_h);
}

/* Finomított gamma skálázás (0.65-ös kitevő a reálisabb eloszláshoz) */
void page2_update_fill_log(lv_obj_t* obj, double current, double max_val) {
    if (!obj || !lv_obj_is_valid(obj)) return;
    if (current <= 0.0) {
        lv_obj_set_size(obj, lv_obj_get_width(obj), 0);
        lv_obj_set_pos(obj, lv_obj_get_x(obj), BAR_H);
        return;
    }
    if (current > max_val) current = max_val;

    double norm = current / max_val;

    // 0.65 kitevő: jóval visszafogottabb alsó tartomány
    double ratio = pow(norm, 0.65);

    int32_t fill_h = (int32_t)(ratio * BAR_H);
    if (fill_h < 3) fill_h = 3;

    lv_obj_set_size(obj, lv_obj_get_width(obj), fill_h);
    lv_obj_set_pos(obj, lv_obj_get_x(obj), BAR_H - fill_h);
}

/* Periodikus telemetria frissítő callback */
static void page2_update_timer_cb(lv_timer_t* timer) {
    if (!s_scr_page2) return;

    pc_telemetry_t pc;
    bool online = pc_service_get_data(&pc);

    char txt[32];
    char val_buf[16], unit_buf[16];

    if (online) {
        /* CPU oszlop */
        lv_label_set_text(pc_name_val, pc.pc_name);
        lv_label_set_text(cpu_type, pc.cpu_name);
        
        snprintf(txt, sizeof(txt), "%.1f", pc.cpu_pct);
        lv_label_set_text(cpu_val, txt);
        lv_label_set_text(cpu_pct, "%");
        page2_update_fill(f_cpu, pc.cpu_pct, 100.0);

        /* RAM oszlop */
        snprintf(txt, sizeof(txt), "%.1f GB", pc.ram_total_gb);
        lv_label_set_text(ram_size, txt);
        
        snprintf(txt, sizeof(txt), "%.0f", pc.ram_pct);
        lv_label_set_text(ram_val, txt);
        lv_label_set_text(ram_pct, "%");
        page2_update_fill(f_ram, pc.ram_pct, 100.0);

        /* DISK oszlop */
        snprintf(txt, sizeof(txt), "C: %.0f GB", pc.disk_total_gb);
        lv_label_set_text(disk_drive, txt);
        
        snprintf(txt, sizeof(txt), "%.0f%%", pc.disk_pct);
        lv_label_set_text(disk_val, txt);

        // Lemez IO dinamikus formázás (B/s, KB/s, MB/s, GB/s)
        split_dynamic_unit(pc.disk_speed_kbs, val_buf, unit_buf);
        lv_label_set_text(disk_io_val, val_buf);
        lv_label_set_text(disk_io_unit, unit_buf);
        // DISK IO átvált a dinamikus logaritmikus sávra!
        page2_update_fill_log(f_disk, pc.disk_speed_kbs, MAX_IO_KBS);

        /* NET oszlop */
        split_dynamic_unit(pc.net_speed_kbs, val_buf, unit_buf);
        lv_label_set_text(net_io_val, val_buf);
        lv_label_set_text(net_io_unit, unit_buf);
        // NET IO átvált a dinamikus logaritmikus sávra!
        page2_update_fill_log(f_net,  pc.net_speed_kbs,  MAX_NET_KBS);

    } else {
        /* Offline állapot - debug információ a gépnév mezőben */
        char dbg[32];
        pc_service_get_debug_str(dbg, sizeof(dbg));
        lv_label_set_text(pc_name_val, dbg);

        lv_label_set_text(cpu_val, "--");
        lv_label_set_text(ram_val, "--");
        lv_label_set_text(disk_io_val, "0.00");
        lv_label_set_text(disk_io_unit, "KB/s");
        lv_label_set_text(net_io_val, "0.00");
        lv_label_set_text(net_io_unit, "KB/s");

        page2_update_fill(f_cpu, 0, 100.0);
        page2_update_fill(f_ram, 0, 100.0);
        page2_update_fill(f_disk, 0, MAX_IO_KBS);
        page2_update_fill(f_net, 0, MAX_NET_KBS);
    }
}

static void screen_page2_delete_cb(lv_event_t* e) {
    if (s_page2_timer) {
        lv_timer_del(s_page2_timer);
        s_page2_timer = NULL;
    }
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

    /* --- IKONOK --- */
    icon_cpu = lv_image_create(cpu_bar);
    lv_image_set_src(icon_cpu, &img_cpu);
    lv_image_set_scale(icon_cpu, 170);
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
    lv_image_set_src(icon_disk, &img_hdd);
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

    /* --- FELSŐ CÍMKÉK --- */
    pc_name_tag = lv_label_create(cpu_bar);
    lv_label_set_text(pc_name_tag, "PC Name:");
    lv_obj_set_style_text_font(pc_name_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(pc_name_tag, LV_ALIGN_TOP_MID, 0, 8);

    pc_name_val = lv_label_create(cpu_bar);
    lv_label_set_text(pc_name_val, "DESKTOP");
    lv_obj_set_style_text_font(pc_name_val, &lv_font_montserrat_14, 0);
    lv_obj_align(pc_name_val, LV_ALIGN_TOP_MID, 0, 26);

    disk_drive = lv_label_create(disk_bar);
    lv_label_set_text(disk_drive, "C: -- GB");
    lv_obj_set_style_text_font(disk_drive, &lv_font_montserrat_14, 0);
    lv_obj_align(disk_drive, LV_ALIGN_TOP_MID, 0, 8);

    disk_val = lv_label_create(disk_bar);
    lv_label_set_text(disk_val, "--%");
    lv_obj_set_style_text_font(disk_val, &lv_font_montserrat_14, 0);
    lv_obj_align(disk_val, LV_ALIGN_TOP_MID, 0, 26);

    /* --- ALSÓ ÉRTÉKCSOPORTOK --- */
    /* CPU */
    cpu_type = lv_label_create(cpu_bar);
    lv_label_set_text(cpu_type, "CPU");
    lv_obj_set_style_text_font(cpu_type, &lv_font_montserrat_14, 0);
    lv_obj_align(cpu_type, LV_ALIGN_CENTER, 0, 45);

    cpu_val = lv_label_create(cpu_bar);
    lv_label_set_text(cpu_val, "--");
    lv_obj_set_style_text_font(cpu_val, &lv_font_montserrat_28, 0);
    lv_obj_align(cpu_val, LV_ALIGN_CENTER, 0, 70);

    cpu_pct = lv_label_create(cpu_bar);
    lv_label_set_text(cpu_pct, "%");
    lv_obj_set_style_text_font(cpu_pct, &lv_font_montserrat_18, 0);
    lv_obj_align(cpu_pct, LV_ALIGN_CENTER, 0, 95);

    /* RAM */
    ram_size = lv_label_create(ram_bar);
    lv_label_set_text(ram_size, "-- GB");
    lv_obj_set_style_text_font(ram_size, &lv_font_montserrat_14, 0);
    lv_obj_align(ram_size, LV_ALIGN_CENTER, 0, 45);

    ram_val = lv_label_create(ram_bar);
    lv_label_set_text(ram_val, "--");
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
    lv_label_set_text(disk_io_val, "0.00");
    lv_obj_set_style_text_font(disk_io_val, &lv_font_montserrat_28, 0);
    lv_obj_align(disk_io_val, LV_ALIGN_CENTER, 0, 70);

    disk_io_unit = lv_label_create(disk_bar);
    lv_label_set_text(disk_io_unit, "KB/s");
    lv_obj_set_style_text_font(disk_io_unit, &lv_font_montserrat_18, 0);
    lv_obj_align(disk_io_unit, LV_ALIGN_CENTER, 0, 95);

    /* NET IO */
    net_io_tag = lv_label_create(net_bar);
    lv_label_set_text(net_io_tag, "NET IO:");
    lv_obj_set_style_text_font(net_io_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(net_io_tag, LV_ALIGN_CENTER, 0, 45);

    net_io_val = lv_label_create(net_bar);
    lv_label_set_text(net_io_val, "0.00");
    lv_obj_set_style_text_font(net_io_val, &lv_font_montserrat_28, 0);
    lv_obj_align(net_io_val, LV_ALIGN_CENTER, 0, 70);

    net_io_unit = lv_label_create(net_bar);
    lv_label_set_text(net_io_unit, "KB/s");
    lv_obj_set_style_text_font(net_io_unit, &lv_font_montserrat_18, 0);
    lv_obj_align(net_io_unit, LV_ALIGN_CENTER, 0, 95);

    /* Szövegszínek beállítása fehérre */
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

    /* Valós telemetria indítása */
    page2_update_timer_cb(NULL);
    s_page2_timer = lv_timer_create(page2_update_timer_cb, 1000, NULL);

    return s_scr_page2;
}