#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperature;
    int   humidity;
    float wind_speed;
    int   wind_direction;
    float uv_index;
    int   weather_code;
    bool  is_day;
    bool  valid;
} weather_data_t;

/* Időjárás lekérdezés futtatása (Core 0 háttérszálból) */
bool weather_service_fetch(const char* city_name, weather_data_t* out_data);

/* A legutoljára sikeresen lekért adatok elérése a GUI számára */
bool weather_service_get_data(weather_data_t* out_data);

/* Periodikus háttérfolyamat hívása (pl. 10 percenként) */
void weather_service_loop(void);

/* Kényszerített frissítés a következő weather_service_loop() hívásig */
void weather_service_force_refresh(void);

#ifdef __cplusplus
}
#endif