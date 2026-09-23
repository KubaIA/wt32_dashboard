#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Klíma üzemmódok */
typedef enum {
    GREE_MODE_AUTO = 0,
    GREE_MODE_COOL = 1,
    GREE_MODE_DRY  = 2,
    GREE_MODE_FAN  = 3,
    GREE_MODE_HEAT = 4
} gree_mode_t;

/* Ventilátor fokozatok */
typedef enum {
    GREE_FAN_AUTO  = 0,
    GREE_FAN_LOW   = 1,
    GREE_FAN_MED   = 2,
    GREE_FAN_HIGH  = 3
} gree_fan_t;

/* Klíma állapota */
typedef struct {
    bool        power;        /* true: BE, false: KI */
    gree_mode_t mode;         /* AUTO, COOL, DRY, FAN, HEAT */
    int8_t      temp_set;     /* Beállított célhőmérséklet (°C, 16-30) */
    int8_t      temp_current; /* Mért belső hőmérséklet (°C) */
    gree_fan_t  fan;          /* Ventilátorsebesség */
    bool        connected;    /* Sikerült-e a kézfogás és él-e a kapcsolat */
    bool        valid;        /* Rendelkezünk-e érvényes mért adattal */
} gree_state_t;

/* Szolgáltatás inicializálása */
void gree_service_init(void);

/* Periodikus háttérciklus (Core 0-n futó net_task hívja) */
void gree_service_loop(void);

/* Legfrissebb adatok lekérése a UI számára (adott indexű klímára, pl. 0 = első klíma) */
bool gree_service_get_state(uint8_t dev_idx, gree_state_t* out_state);

/* Parancs küldése a klímának */
void gree_service_set_power(uint8_t dev_idx, bool on);
void gree_service_set_temp(uint8_t dev_idx, int8_t temp);
void gree_service_set_mode(uint8_t dev_idx, gree_mode_t mode);
void gree_service_set_fan(uint8_t dev_idx, gree_fan_t fan);

#ifdef __cplusplus
}
#endif