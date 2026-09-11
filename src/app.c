#include "app.h"
#include "screen_page1.h"
#include "screen_page2.h"
#include "screen_page3.h"
#include "screen_page4.h"
#include "screen_page5.h"
#include "screen_page6.h"

#define TOTAL_PAGES 6 // Egyelőre az első 6 oldalt kötjük be tesztre
static int s_current_page = 0;

static void gesture_event_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

    if (dir == LV_DIR_LEFT) {
        // Balra húzás -> Következő oldal jobbról befelé
        int next_page = s_current_page + 1;
        if (next_page >= TOTAL_PAGES) next_page = 0; // Ciklikus körbelapozás
        app_set_page(next_page, LV_SCR_LOAD_ANIM_MOVE_LEFT);
    } 
    else if (dir == LV_DIR_RIGHT) {
        // Jobbra húzás -> Előző oldal balról befelé
        int prev_page = s_current_page - 1;
        if (prev_page < 0) prev_page = TOTAL_PAGES - 1;
        app_set_page(prev_page, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    }
}

void app_set_page(int page_index, lv_screen_load_anim_t anim_type) {
    lv_obj_t* new_scr = NULL;

    switch (page_index) {
        case 0: new_scr = screen_page1_create(); break;
        case 1: new_scr = screen_page2_create(); break;
        case 2: new_scr = screen_page3_create(); break;
        case 3: new_scr = screen_page4_create(); break;
        case 4: new_scr = screen_page5_create(); break;
        case 5: new_scr = screen_page6_create(); break;
        default: return;
    }

    if (!new_scr) return;

    s_current_page = page_index;

    // Minden betöltött oldal megkapja a gesztusfigyelőt
    lv_obj_add_event_cb(new_scr, gesture_event_cb, LV_EVENT_GESTURE, NULL);

    // Képernyőváltás animációval (300 ms időtartammal, régi képernyő automatikus törlésével)
    lv_screen_load_anim(new_scr, anim_type, 300, 0, true);
}

void app_init(void) {
    s_current_page = 0;
    app_set_page(0, LV_SCR_LOAD_ANIM_NONE);
}