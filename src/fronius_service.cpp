#include "fronius_service.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static fronius_data_t s_latest_fronius = {0};
static unsigned long s_last_fetch_ms = 0;

bool fronius_service_fetch(const char* ip, fronius_data_t* out_data) {
    if (!ip || strlen(ip) == 0 || !out_data) {
        return false;
    }

    out_data->valid = false;

    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    http.setTimeout(3500);

    /* Fronius PowerFlow API végpont */
    String url = "http://";
    url += ip;
    url += "/solar_api/v1/GetPowerFlowRealtimeData.fcgi";

    if (!http.begin(url)) {
        Serial.println("[FRONIUS] Nem sikerult megnyitni a HTTP kapcsolatot");
        return false;
    }

    int http_code = http.GET();
    if (http_code != HTTP_CODE_OK) {
        Serial.printf("[FRONIUS] HTTP hiba: %d\n", http_code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[FRONIUS] JSON parszolasi hiba: %s\n", err.c_str());
        return false;
    }

    JsonObject site = doc["Body"]["Data"]["Site"];
    if (site.isNull()) {
        Serial.println("[FRONIUS] 'Site' adat nem talalhato a valaszban");
        return false;
    }

    /* Adatok kinyerése (null kezeléssel ha épp nincs termelés vagy lekapcsolt a mező) */
    out_data->power_pv   = site["P_PV"] | 0.0f;
    
    /* A Fronius a ház fogyasztását (P_Load) általában negatív számként adja meg, abszolút értékké tesszük */
    float raw_load = site["P_Load"] | 0.0f;
    out_data->power_load = fabsf(raw_load);

    out_data->power_grid = site["P_Grid"] | 0.0f;

    /* Akkumulátor SOC kiolvasása, ha van az Inverters blokkban */
    JsonObject inverters = doc["Body"]["Data"]["Inverters"];
    out_data->soc = 0.0f;
    out_data->has_battery = false;

    if (!inverters.isNull()) {
        /* Az első inverter eszköz id-je általában "1" */
        if (inverters["1"]["SOC"].is<float>()) {
            out_data->soc = inverters["1"]["SOC"].as<float>();
            out_data->has_battery = true;
        }
    }

    out_data->valid = true;

    Serial.println("[FRONIUS] === INVERTER ADATOK FRISSITVE ===");
    Serial.printf("[FRONIUS] PV: %.0f W, Haz: %.0f W, Grid: %.0f W, SOC: %.0f %%\n",
                  out_data->power_pv, out_data->power_load, out_data->power_grid, out_data->soc);

    return true;
}

bool fronius_service_get_data(fronius_data_t* out_data) {
    if (!out_data || !s_latest_fronius.valid) return false;
    *out_data = s_latest_fronius;
    return true;
}

void fronius_service_loop(void) {
    /* 3 másodpercenként frissítünk (a helyi hálózaton a Fronius gyorsan reagál) */
    unsigned long now = millis();
    if (!s_latest_fronius.valid || (now - s_last_fetch_ms > 3000)) {
        if (strlen(g_cfg.inverter.ip) > 0) {
            fronius_data_t temp_data;
            if (fronius_service_fetch(g_cfg.inverter.ip, &temp_data)) {
                s_latest_fronius = temp_data;
                s_last_fetch_ms = millis();
            } else {
                s_last_fetch_ms = now - 3000 + 1000; /* Hiba esetén 1 mp múlva újrapróbálja */
            }
        }
    }
}

void fronius_service_force_refresh(void) {
    s_last_fetch_ms = 0;
}