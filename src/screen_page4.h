#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* screen_page4_create(void);
void screen_page4_force_update_ip(const char* ip);

#ifdef __cplusplus
}
#endif