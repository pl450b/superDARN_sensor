#ifndef WIFI_H
#define WIFI_H

#include "../../secrets.h"

/* AP Configuration */  
#define WIFI_AP_SSID                WIFI_SSID
#define WIFI_AP_PASSWD              WIFI_PASS
#define WIFI_CHANNEL                6
#define MAX_STA_CONN                18
#define PORT                        3333                    // TCP port number for the server
#define KEEPALIVE_IDLE              40
#define KEEPALIVE_INTERVAL          10
#define KEEPALIVE_COUNT             5
#define SERVER_IP                   "192.168.4.2"  
#define WIFI_RETRY_COUNT            2  // After 10 connect attemps, unit resets
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void tcp_client_task(void *pvParameters);

void tcp_server_task(void *pvParameters);

void wifi_init_sta(void);

#endif // WIFI_H