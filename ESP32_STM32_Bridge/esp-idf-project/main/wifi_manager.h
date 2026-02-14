#pragma once
#include "esp_err.h"

void wifi_manager_init(void);
void wifi_start_ap(void);
void wifi_connect_sta(const char* ssid, const char* password);
bool wifi_is_connected(void);
