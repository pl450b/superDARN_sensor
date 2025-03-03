#ifndef WIFI_H
#define WIFI_H

void tcp_client_task(void *pvParameters);

void tcp_server_task(void *pvParameters);

void wifi_init_sta(void);

#endif // WIFI_H
