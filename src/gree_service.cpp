#include "gree_service.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <mbedtls/base64.h>

#define GREE_UDP_PORT 7000

static const char* GREE_GENERIC_KEY = "a3K8Bx%2r8Y7#xDh";
static const char* GREE_BIND_KEY    = "{yxAHAY_Lm6pbC/<";

/* Gree V2 GCM fix paraméterek */
static const uint8_t GCM_NONCE[12] = {
    0x54, 0x40, 0x78, 0x44, 0x49, 0x67, 0x5A, 0x51, 0x6C, 0x5E, 0x63, 0x13
};
static const uint8_t GCM_AEAD[] = "qualcomm-test";

typedef struct {
    char mac[20];
    char key[40];
    bool bound;
    unsigned long last_req_ms;
    gree_state_t state;
} gree_session_t;

static gree_session_t s_sessions[3];
static WiFiUDP s_udp;
static bool s_udp_initialized = false;

/* ---- Kriptográfia: AES-128-ECB (Scan fázis) ---- */

static String encrypt_pack(const String& input, const char* key_str) {
    if (!key_str || strlen(key_str) == 0) return "";

    size_t in_len = input.length();
    size_t pad_len = 16 - (in_len % 16);
    size_t padded_len = in_len + pad_len;

    uint8_t padded_input[padded_len];
    memcpy(padded_input, input.c_str(), in_len);
    for (size_t i = in_len; i < padded_len; i++) {
        padded_input[i] = (uint8_t)pad_len;
    }

    uint8_t encrypted[padded_len];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*)key_str, 128);

    for (size_t i = 0; i < padded_len; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, padded_input + i, encrypted + i);
    }
    mbedtls_aes_free(&aes);

    size_t b64_len = 0;
    mbedtls_base64_encode(NULL, 0, &b64_len, encrypted, padded_len);
    char b64_output[b64_len + 1];
    mbedtls_base64_encode((unsigned char*)b64_output, b64_len + 1, &b64_len, encrypted, padded_len);
    b64_output[b64_len] = '\0';

    return String(b64_output);
}

static String decrypt_pack(const char* base64_str, const char* key_str) {
    if (!base64_str || !key_str || strlen(key_str) == 0) return "";

    size_t b64_len = strlen(base64_str);
    size_t out_len = 0;
    uint8_t cipher_buf[b64_len];

    if (mbedtls_base64_decode(cipher_buf, sizeof(cipher_buf), &out_len, (const unsigned char*)base64_str, b64_len) != 0) {
        return "";
    }
    if (out_len % 16 != 0 || out_len == 0) {
        return "";
    }

    uint8_t plain_buf[out_len + 1];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, (const unsigned char*)key_str, 128);

    for (size_t i = 0; i < out_len; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, cipher_buf + i, plain_buf + i);
    }
    mbedtls_aes_free(&aes);

    uint8_t pad = plain_buf[out_len - 1];
    if (pad > 16 || pad == 0) return "";
    plain_buf[out_len - pad] = '\0';

    return String((char*)plain_buf);
}

/* ---- Kriptográfia: AES-128-GCM (Bind, Status, Cmd fázis) ---- */

/* Kimenet: out_pack = base64(cipher), out_tag = base64(tag) */
static bool encrypt_pack_gcm(const String& plain, const char* key_str,
                             String& out_pack, String& out_tag) {
    if (!key_str || strlen(key_str) == 0) return false;

    size_t plain_len = plain.length();
    uint8_t cipher[plain_len];
    uint8_t tag[16];

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, (const unsigned char*)key_str, 128);

    int ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plain_len,
                                        GCM_NONCE, sizeof(GCM_NONCE),
                                        GCM_AEAD, sizeof(GCM_AEAD) - 1,
                                        (const unsigned char*)plain.c_str(), cipher,
                                        sizeof(tag), tag);
    mbedtls_gcm_free(&gcm);
    if (ret != 0) return false;

    /* pack = base64(cipher) */
    size_t b64_len = 0;
    mbedtls_base64_encode(NULL, 0, &b64_len, cipher, plain_len);
    char b64_pack[b64_len + 1];
    mbedtls_base64_encode((unsigned char*)b64_pack, b64_len + 1, &b64_len, cipher, plain_len);
    b64_pack[b64_len] = '\0';
    out_pack = String(b64_pack);

    /* tag = base64(tag) */
    b64_len = 0;
    mbedtls_base64_encode(NULL, 0, &b64_len, tag, 16);
    char b64_tag[b64_len + 1];
    mbedtls_base64_encode((unsigned char*)b64_tag, b64_len + 1, &b64_len, tag, 16);
    b64_tag[b64_len] = '\0';
    out_tag = String(b64_tag);

    return true;
}

/* Bemenet: pack_b64 = base64(cipher), tag_b64 = base64(tag) */
static String decrypt_pack_gcm(const char* pack_b64, const char* tag_b64, const char* key_str) {
    if (!pack_b64 || !tag_b64 || !key_str) return "";

    /* pack dekódolása */
    size_t pack_len = strlen(pack_b64);
    uint8_t tmp[pack_len];
    size_t cipher_len = 0;
    if (mbedtls_base64_decode(tmp, sizeof(tmp), &cipher_len,
                              (const unsigned char*)pack_b64, pack_len) != 0) {
        return "";
    }

    /* tag dekódolása */
    size_t tag_len = strlen(tag_b64);
    uint8_t tag[16];
    size_t tag_decoded_len = 0;
    if (mbedtls_base64_decode(tag, sizeof(tag), &tag_decoded_len,
                              (const unsigned char*)tag_b64, tag_len) != 0) {
        return "";
    }
    if (tag_decoded_len != 16) return "";

    uint8_t plain[cipher_len + 1];
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, (const unsigned char*)key_str, 128);

    int ret = mbedtls_gcm_auth_decrypt(&gcm, cipher_len,
                                       GCM_NONCE, sizeof(GCM_NONCE),
                                       GCM_AEAD, sizeof(GCM_AEAD) - 1,
                                       tag, 16,
                                       tmp, plain);
    mbedtls_gcm_free(&gcm);
    if (ret != 0) return "";

    plain[cipher_len] = '\0';
    return String((char*)plain);
}

/* ---- Alaprutinok ---- */

void gree_service_init(void) {
    memset(s_sessions, 0, sizeof(s_sessions));
    for (int i = 0; i < 3; i++) {
        s_sessions[i].state.temp_set = 24;
        s_sessions[i].state.temp_current = 24;
    }
    Serial.println("[GREE] Szolgaltatas elokeszitve.");
}

bool gree_service_get_state(uint8_t dev_idx, gree_state_t* out_state) {
    if (dev_idx >= 3 || !out_state) return false;
    *out_state = s_sessions[dev_idx].state;
    return s_sessions[dev_idx].state.valid;
}

/* ---- Csomagküldés ---- */

static void send_scan(const char* ip) {
    if (!ip || strlen(ip) == 0) return;
    String msg = "{\"t\":\"scan\"}";
    s_udp.beginPacket(ip, GREE_UDP_PORT);
    s_udp.write((const uint8_t*)msg.c_str(), msg.length());
    s_udp.endPacket();
}

static void send_bind(const char* ip, const char* mac) {
    if (!ip || !mac || strlen(mac) == 0) return;

    String inner = "{\"t\":\"bind\",\"mac\":\"" + String(mac) + "\",\"uid\":0}";

    String pack, tag;
    if (!encrypt_pack_gcm(inner, GREE_BIND_KEY, pack, tag)) return;

    String outer = "{\"cid\":\"app\",\"i\":1,\"t\":\"pack\",\"uid\":0,\"tcid\":\"" 
                   + String(mac) + "\",\"tag\":\"" + tag + "\",\"pack\":\"" + pack + "\"}";

    s_udp.beginPacket(ip, GREE_UDP_PORT);
    s_udp.write((const uint8_t*)outer.c_str(), outer.length());
    s_udp.endPacket();

    Serial.printf("[GREE TX BIND GCM] -> %s (MAC: %s)\n", ip, mac);
}

static void send_status_query(uint8_t idx) {
    const char* ip = g_cfg.gree.dev[idx].ip;
    if (!ip || strlen(ip) == 0 || !s_sessions[idx].bound) return;

    String inner = "{\"t\":\"status\",\"mac\":\"" + String(s_sessions[idx].mac) + 
                   "\",\"cols\":[\"Pow\",\"Mod\",\"SetTem\",\"TemSen\",\"Wmd\"]}";

    String pack, tag;
    if (!encrypt_pack_gcm(inner, s_sessions[idx].key, pack, tag)) return;

    String outer = "{\"cid\":\"app\",\"i\":0,\"t\":\"pack\",\"uid\":0,\"tcid\":\"" 
                   + String(s_sessions[idx].mac) + "\",\"tag\":\"" + tag + "\",\"pack\":\"" + pack + "\"}";

    s_udp.beginPacket(ip, GREE_UDP_PORT);
    s_udp.write((const uint8_t*)outer.c_str(), outer.length());
    s_udp.endPacket();
}

/* ---- Beérkező csomagok feldolgozása ---- */

static void handle_incoming_packets(void) {
    int packet_size = s_udp.parsePacket();
    if (packet_size <= 0) return;

    IPAddress remote_ip = s_udp.remoteIP();
    String remote_ip_str = remote_ip.toString();

    static uint8_t rx_buffer[1024];
    int len = s_udp.read(rx_buffer, sizeof(rx_buffer) - 1);
    if (len <= 0) return;
    rx_buffer[len] = '\0';

    int dev_idx = -1;
    for (int i = 0; i < 3; i++) {
        if (remote_ip_str == g_cfg.gree.dev[i].ip) {
            dev_idx = i;
            break;
        }
    }
    if (dev_idx == -1) return;

    Serial.printf("[GREE RX %d] %s\n", dev_idx + 1, (char*)rx_buffer);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (char*)rx_buffer);
    if (err) return;

    const char* t   = doc["t"]   | "";
    const char* tag = doc["tag"] | "";

    /* 1. Nyers bindok válasz */
    if (strcmp(t, "bindok") == 0) {
        const char* new_key = doc["key"] | "";
        if (strlen(new_key) > 0) {
            strncpy(s_sessions[dev_idx].key, new_key, sizeof(s_sessions[dev_idx].key) - 1);
            s_sessions[dev_idx].key[sizeof(s_sessions[dev_idx].key) - 1] = '\0';
            s_sessions[dev_idx].bound = true;
            s_sessions[dev_idx].state.connected = true;
            Serial.printf("[GREE %d] >>> SIKERES NYERS BIND! Kulcs: %s <<<\n", dev_idx + 1, new_key);
        }
        return;
    }

    /* 2. Titkosított 'pack' boríték */
    if (strcmp(t, "pack") == 0) {
        const char* pack = doc["pack"] | "";
        if (strlen(pack) == 0) return;

        if (!s_sessions[dev_idx].bound) {
            String plain;
            /* 1. próba: GCM a GREE_BIND_KEY-jel (bindok válasz) */
            if (strlen(tag) > 0) {
                plain = decrypt_pack_gcm(pack, tag, GREE_BIND_KEY);
            }
            /* 2. próba: ECB a generikus kulccsal (scan/dev válasz) */
            if (plain.length() == 0) {
                plain = decrypt_pack(pack, GREE_GENERIC_KEY);
            }
            Serial.printf("[GREE %d DECRYPT] %s\n", dev_idx + 1, plain.c_str());

            JsonDocument inner_doc;
            if (!deserializeJson(inner_doc, plain)) {
                const char* inner_t = inner_doc["t"] | "";

                if (strcmp(inner_t, "dev") == 0) {
                    const char* mac = inner_doc["mac"] | "";
                    int lock = inner_doc["lock"] | 0;
                    if (lock == 1) {
                        Serial.printf("[GREE %d] A klima masik apphoz kotve (lock=1)!\n", dev_idx + 1);
                    } else if (strlen(mac) > 0) {
                        strncpy(s_sessions[dev_idx].mac, mac, sizeof(s_sessions[dev_idx].mac) - 1);
                        s_sessions[dev_idx].mac[sizeof(s_sessions[dev_idx].mac) - 1] = '\0';
                        Serial.printf("[GREE %d] MAC: %s -> Bind kérés küldése (GCM)...\n", dev_idx + 1, mac);
                        send_bind(remote_ip_str.c_str(), mac);
                    }
                } else if (strcmp(inner_t, "bindok") == 0) {
                    const char* new_key = inner_doc["key"] | "";
                    if (strlen(new_key) > 0) {
                        strncpy(s_sessions[dev_idx].key, new_key, sizeof(s_sessions[dev_idx].key) - 1);
                        s_sessions[dev_idx].key[sizeof(s_sessions[dev_idx].key) - 1] = '\0';
                        s_sessions[dev_idx].bound = true;
                        s_sessions[dev_idx].state.connected = true;
                        s_sessions[dev_idx].last_req_ms = millis();
                        Serial.printf("[GREE %d] >>> SIKERES TITKOSÍTOTT BIND! Kulcs: %s <<<\n", dev_idx + 1, new_key);
                    }
                }
            }
        } else {
            /* Először a GREE_BIND_KEY-jel próbáljuk (bindok nyugtázás) */
            String plain = decrypt_pack_gcm(pack, tag, GREE_BIND_KEY);
            /* Ha nem sikerült, a saját kulccsal próbáljuk (telemetria) */
            if (plain.length() == 0) {
                plain = decrypt_pack_gcm(pack, tag, s_sessions[dev_idx].key);
            }
            Serial.printf("[GREE %d GCM DECRYPT] %s\n", dev_idx + 1, plain.c_str());

            JsonDocument inner_doc;
            if (!deserializeJson(inner_doc, plain)) {
                const char* inner_t = inner_doc["t"] | "";

                /* Ha bindok, csak nyugtázzuk és kilépünk */
                if (strcmp(inner_t, "bindok") == 0) {
                    Serial.printf("[GREE %d] Bindok nyugtazva.\n", dev_idx + 1);
                    return;
                }

                //* Egyébként telemetria feldolgozás */
                JsonArray dat = inner_doc["dat"];
                if (!dat.isNull() && dat.size() >= 4) {
                    s_sessions[dev_idx].state.power        = (dat[0].as<int>() == 1);
                    s_sessions[dev_idx].state.mode         = (gree_mode_t)dat[1].as<int>();
                    s_sessions[dev_idx].state.temp_set     = dat[2].as<int>();
                    int raw_cur = dat[3].as<int>();
                    s_sessions[dev_idx].state.temp_current = (raw_cur > 40) ? (raw_cur - 40) : raw_cur;
                    if (dat.size() >= 5) {
                        s_sessions[dev_idx].state.fan      = (gree_fan_t)dat[4].as<int>();
                    }
                    s_sessions[dev_idx].state.valid        = true;

                    Serial.printf("[GREE %d TELEMETRIA] Be: %s | Cel: %d C | Szoba: %d C\n",
                                dev_idx + 1,
                                s_sessions[dev_idx].state.power ? "BE" : "KI",
                                s_sessions[dev_idx].state.temp_set,
                                s_sessions[dev_idx].state.temp_current);
                }
            }
        }
    }
}

/* ---- Fő ciklus ---- */

void gree_service_loop(void) {
    if (WiFi.status() != WL_CONNECTED) {
        if (s_udp_initialized) {
            s_udp.stop();
            s_udp_initialized = false;
        }
        return;
    }

    if (!s_udp_initialized) {
        s_udp.begin(GREE_UDP_PORT);
        s_udp_initialized = true;
        Serial.println("[GREE] UDP figyeles elinditva (7000).");
    }

    handle_incoming_packets();

    unsigned long now = millis();

    for (int i = 0; i < 3; i++) {
        const char* ip = g_cfg.gree.dev[i].ip;
        if (!ip || strlen(ip) == 0) continue;

        if (now - s_sessions[i].last_req_ms > 10000) {
            s_sessions[i].last_req_ms = now;

            if (!s_sessions[i].bound) {
                if (strlen(s_sessions[i].mac) == 0) {
                    send_scan(ip);
                } else {
                    send_bind(ip, s_sessions[i].mac);
                }
            } else {
                send_status_query(i);
            }
        }
    }
}

/* ---- Parancsküldés ---- */

static void send_param_cmd(uint8_t dev_idx, const char* opt, int val) {
    if (dev_idx >= 3 || !s_sessions[dev_idx].bound) return;

    const char* ip = g_cfg.gree.dev[dev_idx].ip;
    if (!ip || strlen(ip) == 0) return;

    String inner = "{\"t\":\"cmd\",\"mac\":\"" + String(s_sessions[dev_idx].mac) + 
                   "\",\"opt\":[\"" + String(opt) + "\"],\"p\":[" + String(val) + "]}";

    String pack, tag;
    if (!encrypt_pack_gcm(inner, s_sessions[dev_idx].key, pack, tag)) return;

    String outer = "{\"cid\":\"app\",\"i\":0,\"t\":\"pack\",\"uid\":0,\"tcid\":\"" 
                   + String(s_sessions[dev_idx].mac) + "\",\"tag\":\"" + tag + "\",\"pack\":\"" + pack + "\"}";

    s_udp.beginPacket(ip, GREE_UDP_PORT);
    s_udp.write((const uint8_t*)outer.c_str(), outer.length());
    s_udp.endPacket();
}

void gree_service_set_power(uint8_t dev_idx, bool on) {
    if (dev_idx >= 3) return;
    s_sessions[dev_idx].state.power = on;
    send_param_cmd(dev_idx, "Pow", on ? 1 : 0);
}

void gree_service_set_temp(uint8_t dev_idx, int8_t temp) {
    if (dev_idx >= 3) return;
    if (temp < 16) temp = 16;
    if (temp > 30) temp = 30;
    s_sessions[dev_idx].state.temp_set = temp;
    send_param_cmd(dev_idx, "SetTem", temp);
}

void gree_service_set_mode(uint8_t dev_idx, gree_mode_t mode) {
    if (dev_idx >= 3) return;
    s_sessions[dev_idx].state.mode = mode;
    send_param_cmd(dev_idx, "Mod", (int)mode);
}

void gree_service_set_fan(uint8_t dev_idx, gree_fan_t fan) {
    if (dev_idx >= 3) return;
    s_sessions[dev_idx].state.fan = fan;
    send_param_cmd(dev_idx, "Wmd", (int)fan);
}