#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Hálózati háttérfolyamat (Wi-Fi + NTP) indítása Core 0-on */
void net_service_init(void);

/* Segédfüggvények a kapcsolat és az idő állapotának lekérdezésére */
bool net_service_is_wifi_connected(void);
bool net_service_is_time_synced(void);

#ifdef __cplusplus
}
#endif