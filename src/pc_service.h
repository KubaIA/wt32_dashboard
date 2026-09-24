#ifndef PC_SERVICE_H
#define PC_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool online;              // Van-e friss adat az elmúlt 4 másodpercben
    char pc_name[32];         // Számítógép neve
    char cpu_name[32];        // CPU típusa
    float cpu_pct;            // CPU terhelés (0.0 - 100.0 %)
    
    float ram_total_gb;       // Összes RAM (GB)
    float ram_pct;            // Használt RAM (0.0 - 100.0 %)
    
    float disk_total_gb;      // C: meghajtó mérete (GB)
    float disk_pct;           // C: meghajtó telítettsége (0.0 - 100.0 %)
    float disk_speed_kbs;     // Olvasás + írás sebesség összesen (KB/s)
    
    float net_speed_kbs;      // RX + TX sebesség összesen (KB/s)
} pc_telemetry_t;

void pc_service_init(uint16_t port);
void pc_service_loop(void);
bool pc_service_get_data(pc_telemetry_t* out);

#ifdef __cplusplus
}
#endif

#endif // PC_SERVICE_H