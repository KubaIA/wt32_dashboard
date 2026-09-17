#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ip[16];
} gree_device_t;

typedef struct {
    struct {
        char ssid[64];
        char password[64];
    } wifi;

    struct {
        char city[64];
    } weather;

    struct {
        char ip[16];
    } inverter;

    struct {
        gree_device_t dev[3];
    } gree;

    struct {
        uint8_t brightness;
    } display;

} app_config_t;

extern app_config_t g_cfg;

void config_init_defaults(app_config_t* cfg);
bool config_load(app_config_t* cfg);
bool config_save(const app_config_t* cfg);

#ifdef __cplusplus
}
#endif