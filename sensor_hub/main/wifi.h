#ifndef WIFI_H
#define WIFI_H

void tcp_client_task(void *pvParameters);

void tcp_server_task(void *pvParameters);

void init_wifi_ap(void);

#endif WIFI_H
