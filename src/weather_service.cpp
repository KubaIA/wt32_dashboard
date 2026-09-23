#include "weather_service.h"
#include "config.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool weather_service_fetch(const char* city_name, weather_data_t* out_data) {
    if (!city_name || strlen(city_name) == 0 || !out_data) {
        return false;
    }

    out_data->valid = false;

    WiFiClientSecure client;
    client.setInsecure(); // Nem ellenőrizzük a CA láncot a memóriatakarékosság miatt

    HTTPClient https;
    https.setTimeout(6000);

    /* 1. LÉPÉS: Geokódolás (Városnév -> Szélesség, Hosszúság) */
    String geo_url = "https://geocoding-api.open-meteo.com/v1/search?name=";
    geo_url += city_name;
    geo_url += "&count=1&language=en&format=json";

    Serial.printf("[WEATHER] Geokodolas inditasa: %s...\n", city_name);

    if (!https.begin(client, geo_url)) {
        Serial.println("[WEATHER] Nem sikerult inicializalni a HTTPS kapcsolatot (Geocoding)");
        return false;
    }

    int http_code = https.GET();
    if (http_code != HTTP_CODE_OK) {
        Serial.printf("[WEATHER] Geocoding HTTP hiba: %d\n", http_code);
        https.end();
        return false;
    }

    String geo_payload = https.getString();
    https.end();

    JsonDocument geo_doc;
    DeserializationError err = deserializeJson(geo_doc, geo_payload);
    if (err) {
        Serial.printf("[WEATHER] Geocoding JSON parszolasi hiba: %s\n", err.c_str());
        return false;
    }

    if (!geo_doc["results"] || geo_doc["results"].size() == 0) {
        Serial.println("[WEATHER] A keresett varos nem talalhato!");
        return false;
    }

    float lat = geo_doc["results"][0]["latitude"];
    float lon = geo_doc["results"][0]["longitude"];
    Serial.printf("[WEATHER] Koordinatak megtalalva: Lat: %.4f, Lon: %.4f\n", lat, lon);

    /* 2. LÉPÉS: Időjárási adatok lekérdezése (Forecast API) */
    String forecast_url = "https://api.open-meteo.com/v1/forecast?latitude=";
    forecast_url += String(lat, 4);
    forecast_url += "&longitude=";
    forecast_url += String(lon, 4);
    forecast_url += "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,wind_direction_10m,weather_code,uv_index,is_day";

    Serial.println("[WEATHER] Idojaras lekerdezese...");
    if (!https.begin(client, forecast_url)) {
        Serial.println("[WEATHER] Nem sikerult inicializalni a HTTPS kapcsolatot (Forecast)");
        return false;
    }

    http_code = https.GET();
    if (http_code != HTTP_CODE_OK) {
        Serial.printf("[WEATHER] Forecast HTTP hiba: %d\n", http_code);
        https.end();
        return false;
    }

    String forecast_payload = https.getString();
    https.end();

    JsonDocument forecast_doc;
    err = deserializeJson(forecast_doc, forecast_payload);
    if (err) {
        Serial.printf("[WEATHER] Forecast JSON parszolasi hiba: %s\n", err.c_str());
        return false;
    }

    JsonObject current = forecast_doc["current"];
    if (current.isNull()) {
        Serial.println("[WEATHER] 'current' mező hianyzik a JSON valaszbol");
        return false;
    }

    out_data->temperature  = current["temperature_2m"] | 0.0f;
    out_data->humidity     = current["relative_humidity_2m"] | 0;
    out_data->wind_speed   = current["wind_speed_10m"] | 0.0f;
    out_data->wind_direction = current["wind_direction_10m"] | 0;
    out_data->uv_index     = current["uv_index"] | 0.0f;
    out_data->weather_code = current["weather_code"] | 0;
    out_data->is_day       = (current["is_day"] | 1) == 1;
    out_data->valid        = true;

    Serial.println("[WEATHER] === SIKERES IDOJARAS LEKERDEZES ===");
    Serial.printf("[WEATHER] Homerséklet: %.1f C\n", out_data->temperature);
    Serial.printf("[WEATHER] Paratartalom: %d %%\n", out_data->humidity);
    Serial.printf("[WEATHER] Szelsebesseg: %.1f km/h  %d°\n", out_data->wind_speed, out_data->wind_direction);
    Serial.printf("[WEATHER] UV index:     %.1f\n", out_data->uv_index);
    Serial.printf("[WEATHER] WMO kod:      %d\n", out_data->weather_code);
    Serial.printf("[WEATHER] Nappal van:   %s\n", out_data->is_day ? "IGEN" : "NEM");

    return true;
}

/* Legfrissebb érvényes mérési adatok tárolója */
static weather_data_t s_latest_weather = {0};
static unsigned long s_last_fetch_ms = 0;

bool weather_service_get_data(weather_data_t* out_data) {
    if (!out_data || !s_latest_weather.valid) return false;
    *out_data = s_latest_weather;
    return true;
}

void weather_service_loop(void) {
    if (WiFi.status() != WL_CONNECTED) return;

    static unsigned long s_next_run = 0;
    unsigned long now = millis();

    if (now < s_next_run) return;

    weather_data_t temp_data;
    const char* city = (strlen(g_cfg.weather.city) > 0) ? g_cfg.weather.city : "Budapest";
    
    if (weather_service_fetch(city, &temp_data)) {
        s_latest_weather = temp_data;
        s_next_run = now + (15 * 60 * 1000); // Siker: 15 perc
    } else {
        s_next_run = now + (60 * 1000);      // Hiba: szigorúan 1 percig semmit sem csinál!
        Serial.println("[WEATHER] Hiba tortent, varakozas 1 percig...");
    }
}

void weather_service_force_refresh(void) {
    s_last_fetch_ms = 0;
}