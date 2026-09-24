#include "pc_service.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>

static pc_telemetry_t s_pc_data;
static bool s_pc_inited = false;
static uint32_t s_last_packet_ms = 0;

static volatile uint32_t s_raw_byte_count = 0;
static volatile uint32_t s_packet_count = 0;
static char s_diag_status[16] = "INIT";

static void parse_pc_json(char* json_str) {
    if (!json_str || strlen(json_str) < 5) {
        strncpy(s_diag_status, "EMPTY", sizeof(s_diag_status));
        return;
    }

    char* start = strchr(json_str, '{');
    if (!start) {
        strncpy(s_diag_status, "NO_BRACE", sizeof(s_diag_status));
        return;
    }

    char* end = strrchr(start, '}');
    if (!end || end < start) {
        strncpy(s_diag_status, "INCOMPL", sizeof(s_diag_status));
        return;
    }
    // Levágjuk a felesleges karaktereket a záró kapcsos után
    *(end + 1) = '\0';

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, start);
    if (err) {
        snprintf(s_diag_status, sizeof(s_diag_status), "E:%d", (int)err.code());
        return;
    }

    s_packet_count++;
    strncpy(s_diag_status, "OK", sizeof(s_diag_status));

    const char* host = doc["computer"] | "PC";
    snprintf(s_pc_data.pc_name, sizeof(s_pc_data.pc_name), "%s", host);

    const char* c_name = doc["cpu"]["name"] | "CPU";
    strncpy(s_pc_data.cpu_name, c_name, sizeof(s_pc_data.cpu_name) - 1);
    s_pc_data.cpu_name[sizeof(s_pc_data.cpu_name) - 1] = '\0';

    s_pc_data.cpu_pct = doc["cpu"]["usage_pct"].as<float>();
    s_pc_data.ram_total_gb = doc["ram"]["total_gb"].as<float>();
    s_pc_data.ram_pct = doc["ram"]["used_pct"].as<float>();
    s_pc_data.disk_total_gb = doc["disk"]["total_gb"].as<float>();
    s_pc_data.disk_pct = doc["disk"]["used_pct"].as<float>();

    float disk_r = doc["disk_io"]["read_kb_s"].as<float>();
    float disk_w = doc["disk_io"]["write_kb_s"].as<float>();
    s_pc_data.disk_speed_kbs = disk_r + disk_w;

    float net_rx = doc["net"]["rx_kb_s"].as<float>();
    float net_tx = doc["net"]["tx_kb_s"].as<float>();
    s_pc_data.net_speed_kbs = net_rx + net_tx;

    s_pc_data.online = true;
    s_last_packet_ms = millis();
}

static void pc_rx_task(void* arg) {
    static char rx_line[1024];

    // Soros olvasási timeout minimálisra vétele, hogy ne blokkoljon
    Serial.setTimeout(20);

    while (1) {
        // 6 másodperces türelmi idő
        if (s_pc_data.online && (millis() - s_last_packet_ms > 6000)) {
            s_pc_data.online = false;
            strncpy(s_diag_status, "TIMEOUT", sizeof(s_diag_status));
        }

        if (Serial.available()) {
            // Egy teljes sort olvasunk be a következő \n karakterig
            size_t bytes_read = Serial.readBytesUntil('\n', rx_line, sizeof(rx_line) - 1);
            if (bytes_read > 0) {
                s_raw_byte_count += bytes_read;
                rx_line[bytes_read] = '\0';

                // Ha van benne adat, feldolgozzuk
                if (bytes_read > 5) {
                    parse_pc_json(rx_line);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void pc_service_init(uint16_t port) {
    (void)port;
    if (s_pc_inited) return;
    s_pc_inited = true;

    memset(&s_pc_data, 0, sizeof(s_pc_data));
    s_pc_data.online = false;

    // 6144 bájt veremméret a biztonságos JSON deserializációhoz
    xTaskCreatePinnedToCore(
        pc_rx_task,
        "pc_rx_task",
        6144,
        NULL,
        2,
        NULL,
        0
    );
}

void pc_service_loop(void) {}

bool pc_service_get_data(pc_telemetry_t* out) {
    if (!out) return false;
    *out = s_pc_data;
    return s_pc_data.online;
}

void pc_service_get_debug_str(char* buf, size_t max_len) {
    // Tömör kiírás: pl. "P:85 E:2" vagy "P:85 TIMEOUT"
    snprintf(buf, max_len, "P:%lu %s", (unsigned long)s_packet_count, s_diag_status);
}