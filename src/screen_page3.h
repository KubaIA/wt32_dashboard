#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* screen_page3_create(void);
void screen_page3_force_update_location(const char* city);

#ifdef __cplusplus
}
#endif