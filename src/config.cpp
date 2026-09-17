#include "config.h"
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

/* WT32-SC01 PLUS hivatalos SD-kártya GPIO kiosztás */
#define SD_CLK_PIN   39
#define SD_MOSI_PIN  40
#define SD_CS_PIN    41
#define SD_MISO_PIN  38

#define CONFIG_FILE_PATH "/config.json"

app_config_t g_cfg;
static bool s_sd_ready = false;
static SPIClass s_sd_spi(FSPI); // ESP32-S3-on a szabad SPI busz a FSPI

static bool init_sd_card(void) {
    if (s_sd_ready) return true;

    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

    // s_sd_spi.begin(sck, miso, mosi, ss) sorrend az Arduino SPI-ben!
    s_sd_spi.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!SD.begin(SD_CS_PIN, s_sd_spi, 4000000)) {
        Serial.println("[SD] HIBA: SD kartya nem inicializalhato!");
        s_sd_ready = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] Nincs kartya a foglalatban!");
        s_sd_ready = false;
        return false;
    }

    Serial.println("[SD] SD kartya sikeresen csatolva.");
    s_sd_ready = true;
    return true;
}

void config_init_defaults(app_config_t* cfg) {
    memset(cfg, 0, sizeof(app_config_t));

    strncpy(cfg->wifi.ssid, "Otthoni_WiFi", sizeof(cfg->wifi.ssid) - 1);
    strncpy(cfg->wifi.password, "TitkosJelszo123", sizeof(cfg->wifi.password) - 1);
    strncpy(cfg->weather.city, "Budapest", sizeof(cfg->weather.city) - 1);
    strncpy(cfg->inverter.ip, "192.168.1.150", sizeof(cfg->inverter.ip) - 1);

    strncpy(cfg->gree.dev[0].ip, "192.168.1.101", sizeof(cfg->gree.dev[0].ip) - 1);
    strncpy(cfg->gree.dev[1].ip, "192.168.1.102", sizeof(cfg->gree.dev[1].ip) - 1);
    strncpy(cfg->gree.dev[2].ip, "192.168.1.103", sizeof(cfg->gree.dev[2].ip) - 1);

    cfg->display.brightness = 80;
}

bool config_load(app_config_t* cfg) {
    if (!init_sd_card()) {
        config_init_defaults(cfg);
        return false;
    }

    if (!SD.exists(CONFIG_FILE_PATH)) {
        Serial.println("[CONFIG] Nincs mentes az SD-n, alapertelmezett adatok mentese...");
        config_init_defaults(cfg);
        return config_save(cfg);
    }

    File file = SD.open(CONFIG_FILE_PATH, FILE_READ);
    if (!file) {
        Serial.println("[CONFIG] HIBA a fajl megnyitasakor!");
        config_init_defaults(cfg);
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[CONFIG] JSON hiba: %s\n", error.c_str());
        config_init_defaults(cfg);
        return false;
    }

    strncpy(cfg->wifi.ssid, doc["wifi"]["ssid"] | "Otthoni_WiFi", sizeof(cfg->wifi.ssid) - 1);
    strncpy(cfg->wifi.password, doc["wifi"]["password"] | "", sizeof(cfg->wifi.password) - 1);
    strncpy(cfg->weather.city, doc["weather"]["city"] | "Budapest", sizeof(cfg->weather.city) - 1);
    strncpy(cfg->inverter.ip, doc["inverter"]["ip"] | "192.168.1.150", sizeof(cfg->inverter.ip) - 1);

    for (int i = 0; i < 3; i++) {
        char default_ip[16];
        snprintf(default_ip, sizeof(default_ip), "192.168.1.10%d", i + 1);
        strncpy(cfg->gree.dev[i].ip, doc["gree"]["dev"][i]["ip"] | default_ip, sizeof(cfg->gree.dev[i].ip) - 1);
    }

    cfg->display.brightness = doc["display"]["brightness"] | 80;

    Serial.println("[CONFIG] Konfiguracio betoltve az SD kartyarol.");
    return true;
}

bool config_save(const app_config_t* cfg) {
    if (!init_sd_card()) return false;

    File file = SD.open(CONFIG_FILE_PATH, FILE_WRITE);
    if (!file) {
        Serial.println("[CONFIG] HIBA a fajl irasra nyitasakor!");
        return false;
    }

    JsonDocument doc;
    doc["wifi"]["ssid"] = cfg->wifi.ssid;
    doc["wifi"]["password"] = cfg->wifi.password;
    doc["weather"]["city"] = cfg->weather.city;
    doc["inverter"]["ip"] = cfg->inverter.ip;

    for (int i = 0; i < 3; i++) {
        doc["gree"]["dev"][i]["ip"] = cfg->gree.dev[i].ip;
    }

    doc["display"]["brightness"] = cfg->display.brightness;

    if (serializeJsonPretty(doc, file) == 0) {
        Serial.println("[CONFIG] HIBA a JSON irasakor!");
        file.close();
        return false;
    }

    file.close();
    Serial.println("[CONFIG] Konfiguracio sikeresen elmentve az SD-re.");
    return true;
}