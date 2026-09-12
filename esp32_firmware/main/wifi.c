/*
 * This file is part of AmigaClock.
 *
 * Copyright (C) 2026 Andreas Signer <asigner@gmail.com>
 *
 * AmigaClock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AmigaClock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AmigaClock.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#include "wifi.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "nvs.h"

static const char *TAG = "wifi";

#define WIFI_NVS_NAMESPACE "amigawifi"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASS_KEY "password"
#define NTP_SERVER "pool.ntp.org"

static esp_netif_t *s_netif = NULL;
static bool s_wifi_initialized = false;
static bool s_sntp_initialized = false;
static char s_wifi_ssid[33] = {0};
static char s_wifi_pass[65] = {0};
static wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;
static uint8_t s_wifi_ip[4] = {0};

static void wifi_start_sntp(void)
{
    if (s_sntp_initialized) {
        return;
    }

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    esp_sntp_init();
    s_sntp_initialized = true;
    ESP_LOGI(TAG, "SNTP started with server %s", NTP_SERVER);
}

static void wifi_store_credentials(const char *ssid, const char *password)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return;
    }

    if (ssid != NULL) {
        nvs_set_str(handle, WIFI_SSID_KEY, ssid);
    }
    if (password != NULL) {
        nvs_set_str(handle, WIFI_PASS_KEY, password);
    }
    nvs_commit(handle);
    nvs_close(handle);
}

static void wifi_load_credentials(void)
{
    nvs_handle_t handle = 0;
    size_t ssid_len = sizeof(s_wifi_ssid);
    size_t pass_len = sizeof(s_wifi_pass);

    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return;
    }

    memset(s_wifi_ssid, 0, sizeof(s_wifi_ssid));
    memset(s_wifi_pass, 0, sizeof(s_wifi_pass));
    err = nvs_get_str(handle, WIFI_SSID_KEY, s_wifi_ssid, &ssid_len);
    if (err == ESP_OK) {
        err = nvs_get_str(handle, WIFI_PASS_KEY, s_wifi_pass, &pass_len);
    }
    nvs_close(handle);

    if (err != ESP_OK) {
        s_wifi_ssid[0] = '\0';
        s_wifi_pass[0] = '\0';
    }
}

static void wifi_connect_now(void)
{
    if (s_wifi_ssid[0] == '\0') {
        ESP_LOGW(TAG, "No WiFi credentials configured");
        return;
    }

    if (!s_wifi_initialized) {
        ESP_LOGW(TAG, "WiFi not initialized yet; cannot connect");
        return;
    }

    wifi_config_t wifi_cfg = {0};
    memcpy(wifi_cfg.sta.ssid, s_wifi_ssid, strnlen(s_wifi_ssid, sizeof(wifi_cfg.sta.ssid)));
    memcpy(wifi_cfg.sta.password, s_wifi_pass, strnlen(s_wifi_pass, sizeof(wifi_cfg.sta.password)));
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_STATE) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
        return;
    }

    s_wifi_status = WIFI_STATUS_CONNECTING;
    ESP_LOGI(TAG, "Trying to connect to WiFi SSID: %s", s_wifi_ssid);
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                s_wifi_status = WIFI_STATUS_CONNECTING;
                ESP_LOGI(TAG, "WiFi STA started");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                s_wifi_status = WIFI_STATUS_CONNECTED;
                ESP_LOGI(TAG, "WiFi connected");
                break;
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *)event_data;
                s_wifi_status = WIFI_STATUS_DISCONNECTED;
                memset(s_wifi_ip, 0, sizeof(s_wifi_ip));
                ESP_LOGW(TAG, "WiFi disconnected, reason=%d", disconn ? disconn->reason : -1);
                if (s_wifi_ssid[0] != '\0') {
                    esp_wifi_connect();
                }
                break;
            }
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_wifi_status = WIFI_STATUS_CONNECTED;
        s_wifi_ip[0] = (event->ip_info.ip.addr >> 0) & 0xFF;
        s_wifi_ip[1] = (event->ip_info.ip.addr >> 8) & 0xFF;
        s_wifi_ip[2] = (event->ip_info.ip.addr >> 16) & 0xFF;
        s_wifi_ip[3] = (event->ip_info.ip.addr >> 24) & 0xFF;
        ESP_LOGI(TAG, "IP: %d.%d.%d.%d", s_wifi_ip[0], s_wifi_ip[1], s_wifi_ip[2], s_wifi_ip[3]);
        wifi_start_sntp();
    }
}

void wifi_init(void)
{
    if (s_wifi_initialized) {
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_netif = esp_netif_create_default_wifi_sta();
    if (s_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi netif");
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    s_wifi_initialized = true;

    wifi_load_credentials();
    if (s_wifi_ssid[0] != '\0') {
        wifi_connect_now();
    }
}

void wifi_apply_config(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) {
        return;
    }

    snprintf(s_wifi_ssid, sizeof(s_wifi_ssid), "%s", ssid);
    snprintf(s_wifi_pass, sizeof(s_wifi_pass), "%s", password);
    wifi_store_credentials(s_wifi_ssid, s_wifi_pass);
    wifi_init();
    wifi_connect_now();
}

void wifi_get_status(wifi_status_info_t *status)
{
    if (status == NULL) {
        return;
    }

    memset(status, 0, sizeof(*status));
    status->status = s_wifi_status;
    snprintf(status->ssid, sizeof(status->ssid), "%s", s_wifi_ssid);
    memcpy(status->ip, s_wifi_ip, sizeof(s_wifi_ip));
}
