#include "net_service.h"
#include "config.h"
#include "weather_service.h"
#include "fronius_service.h"
#include "gree_service.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>

static bool s_wifi_connected = false;
static bool s_time_synced = false;

/* NTP szerverek */
static const char* ntp_server1 = "pool.ntp.org";
static const char* ntp_server2 = "time.nist.gov";

/* Magyarországi időzóna (CET/CEST automatikus nyári/téli átállással) */
static const char* default_tz = "CET-1CEST,M3.5.0,M10.5.0/3";

/* NTP szinkronizációs callback (ESP-IDF) */
static void time_sync_notification_cb(struct timeval *tv) {
    Serial.println("[NTP] Ido szinkronizalva az idoszerverrol!");
    s_time_synced = true;
}

/* Hálózati szál a Core 0 processzormagon */
static void net_task(void *pvParameters) {
    Serial.println("[NET] Halozati task elindult a Core 0-n");

    /* Wi-Fi mód beállítása állomás (Station) üzemmódra */
    WiFi.mode(WIFI_STA);

    while (1) {
        /* Ha nincs beállítva SSID, várunk */
        if (strlen(g_cfg.wifi.ssid) == 0) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        /* Ha megszakadt vagy még nem kapcsolódott a Wi-Fi */
        if (WiFi.status() != WL_CONNECTED) {
            s_wifi_connected = false;
            Serial.printf("[NET] Csatlakozas a Wi-Fi-hez: %s...\n", g_cfg.wifi.ssid);
            IPAddress dns1(8, 8, 8, 8);
            IPAddress dns2(1, 1, 1, 1);
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns1, dns2); 
            WiFi.begin(g_cfg.wifi.ssid, g_cfg.wifi.password);

            int retry = 0;
            while (WiFi.status() != WL_CONNECTED && retry < 20) {
                vTaskDelay(pdMS_TO_TICKS(500));
                Serial.print(".");
                retry++;
            }
            Serial.println();

            if (WiFi.status() == WL_CONNECTED) {
                s_wifi_connected = true;
                Serial.printf("[NET] Wi-Fi sikeresen kapcsolodva! IP: %s\n", WiFi.localIP().toString().c_str());

                /* NTP konfiguráció indítása az első sikeres kapcsolatkor */
                if (!s_time_synced) {
                    Serial.println("[NTP] Idoszinkronizacio inditasa...");
                    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
                    configTzTime(default_tz, ntp_server1, ntp_server2);
                }

                /* Egyszeri teszt lekérdezés a beállított városra */
                weather_data_t w_data;
                const char* target_city = (strlen(g_cfg.weather.city) > 0) ? g_cfg.weather.city : "Budapest";
                weather_service_fetch(target_city, &w_data);
                
            } else {
                Serial.println("[NET] Wi-Fi csatlakozas sikertelen. Ujraprobalas 5 masodperc mulva.");
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        } else {
            s_wifi_connected = true;
            weather_service_loop();
            fronius_service_loop();
            gree_service_loop();
            vTaskDelay(pdMS_TO_TICKS(3000)); // Kapcsolat ellenőrzése 3 másodpercenként
        }
    }
}

void net_service_init(void) {
    /* 10240 bájtos stack méret, 1-es prioritás, Core 0 */
    xTaskCreatePinnedToCore(
        net_task,
        "net_task",
        10240,
        NULL,
        1,
        NULL,
        0
    );
}

bool net_service_is_wifi_connected(void) {
    return s_wifi_connected;
}

bool net_service_is_time_synced(void) {
    return s_time_synced;
}