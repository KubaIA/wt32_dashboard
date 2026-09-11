#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_init(void);
void app_set_page(int page_index, lv_screen_load_anim_t anim_type);

#ifdef __cplusplus
}
#endif