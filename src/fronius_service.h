#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float power_pv;   /* Watt (Napelem) */
    float power_load; /* Watt (Fogyasztás) */
    float power_grid; /* Watt (+ import, - export) */
    float soc;        /* Akkumulátor % (0-100) */
    bool  has_battery;
    bool  valid;
} fronius_data_t;

/* Adatok lekérése az invertertől (Core 0 szálból hívandó) */
bool fronius_service_fetch(const char* ip, fronius_data_t* out_data);

/* Legutóbbi érvényes adatok átadása a UI felé */
bool fronius_service_get_data(fronius_data_t* out_data);

/* Periodikus háttérciklus kezelő */
void fronius_service_loop(void);

/* Kényszerített azonnali frissítés (pl. IP váltáskor) */
void fronius_service_force_refresh(void);

#ifdef __cplusplus
}
#endif