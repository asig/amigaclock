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

 #include "ble_services.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>

#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "amiga_clock_ui.h"
#include "lcd_panel.h"
#include "wifi.h"

static const char *TAG = "ble_services";
#define DEVICE_NAME "AmigaClock"

#define WIFI_SERVICE_UUID 0xFFF0
#define WIFI_WIFI_CFG_CHAR_UUID 0xFFF1
#define WIFI_STATUS_CHAR_UUID 0xFFF2
#define DISPLAY_CONFIG_CHAR_UUID 0xFFF3
#define BACKLIGHT_CHAR_UUID 0xFFF4

static uint8_t s_own_addr_type;

static void fill_current_time_bytes(uint8_t buf[10])
{
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    uint16_t year = t.tm_year + 1900;
    buf[0] = year & 0xFF;
    buf[1] = (year >> 8) & 0xFF;
    buf[2] = t.tm_mon + 1;
    buf[3] = t.tm_mday;
    buf[4] = t.tm_hour;
    buf[5] = t.tm_min;
    buf[6] = t.tm_sec;
    buf[7] = (t.tm_wday == 0) ? 7 : t.tm_wday;
    buf[8] = 0;
    buf[9] = 0;
}

static int set_time_from_buffer(const uint8_t *buf, size_t len)
{
    if (len < 7) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    struct tm t = {0};
    uint16_t year = buf[0] | (buf[1] << 8);
    t.tm_year = year - 1900;
    t.tm_mon = buf[2] - 1;
    t.tm_mday = buf[3];
    t.tm_hour = buf[4];
    t.tm_min = buf[5];
    t.tm_sec = buf[6];

    time_t epoch = mktime(&t);
    if (epoch < 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    struct timeval tv = {.tv_sec = epoch, .tv_usec = 0};
    settimeofday(&tv, NULL);
    ESP_LOGI(TAG, "Zeit via BLE gesetzt: %04d-%02d-%02d %02d:%02d:%02d",
             year, buf[2], buf[3], buf[4], buf[5], buf[6]);
    return 0;
}

static void fill_wifi_status_bytes(uint8_t *buf, size_t *out_len)
{
    wifi_status_info_t info;
    wifi_get_status(&info);

    size_t ssid_len = strlen(info.ssid);
    size_t total = 1 + 1 + ssid_len + 4;
    memset(buf, 0, total);
    buf[0] = (uint8_t)info.status;
    buf[1] = (uint8_t)ssid_len;
    if (ssid_len > 0) {
        memcpy(&buf[2], info.ssid, ssid_len);
    }
    if (info.status == WIFI_STATUS_CONNECTED) {
        memcpy(&buf[2 + ssid_len], info.ip, 4);
    }
    *out_len = total;
}

static int gatt_chr_access_current_time(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint8_t buf[10] = {0};
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len > sizeof(buf)) {
            len = sizeof(buf);
        }
        if (os_mbuf_copydata(ctxt->om, 0, len, buf) != 0 || len < 7) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        return set_time_from_buffer(buf, len);
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t buf[10] = {0};
        fill_current_time_bytes(buf);
        os_mbuf_append(ctxt->om, buf, sizeof(buf));
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_chr_access_wifi_config(uint16_t conn_handle, uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len < 2) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    uint8_t *buf = malloc(len);
    if (buf == NULL) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (os_mbuf_copydata(ctxt->om, 0, len, buf) != 0) {
        free(buf);
        return BLE_ATT_ERR_UNLIKELY;
    }

    uint8_t ssid_len = buf[0];
    uint8_t pass_len = buf[1];
    if (ssid_len == 0 || ssid_len > 31 || pass_len > 63 || len != (2 + ssid_len + pass_len)) {
        free(buf);
        return BLE_ATT_ERR_UNLIKELY;
    }

    char ssid[32] = {0};
    char pass[64] = {0};
    memcpy(ssid, &buf[2], ssid_len);
    memcpy(pass, &buf[2 + ssid_len], pass_len);

    wifi_apply_config(ssid, pass);
    free(buf);
    return 0;
}

static int gatt_chr_access_wifi_status(uint16_t conn_handle, uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    uint8_t buf[64] = {0};
    size_t len = 0;
    fill_wifi_status_bytes(buf, &len);
    os_mbuf_append(ctxt->om, buf, len);
    return 0;
}

static int gatt_chr_access_display_config(uint16_t conn_handle, uint16_t attr_handle,
                                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint8_t buf[2] = {0};
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len != sizeof(buf) || os_mbuf_copydata(ctxt->om, 0, len, buf) != 0 ||
            buf[0] > 1 || buf[1] > 1) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        amiga_clock_ui_configure(buf[0] != 0, buf[1] != 0);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        bool show_date = false;
        bool show_seconds = false;
        amiga_clock_ui_get_config(&show_date, &show_seconds);
        uint8_t buf[2] = {show_date, show_seconds};
        os_mbuf_append(ctxt->om, buf, sizeof(buf));
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_chr_access_backlight(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint8_t buf[1] = {0};
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len != sizeof(buf) || os_mbuf_copydata(ctxt->om, 0, len, buf) != 0 ||
            buf[0] > 100) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        lcd_panel_set_backlight(buf[0]);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t buf[1] = {lcd_panel_get_backlight()};
        os_mbuf_append(ctxt->om, buf, sizeof(buf));
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x1805),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2A2B),
                .access_cb = gatt_chr_access_current_time,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            { 0 },
        },
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(WIFI_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(WIFI_WIFI_CFG_CHAR_UUID),
                .access_cb = gatt_chr_access_wifi_config,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(WIFI_STATUS_CHAR_UUID),
                .access_cb = gatt_chr_access_wifi_status,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(DISPLAY_CONFIG_CHAR_UUID),
                .access_cb = gatt_chr_access_display_config,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(BACKLIGHT_CHAR_UUID),
                .access_cb = gatt_chr_access_backlight,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            { 0 },
        },
    },
    { 0 },
};

static int gatt_svr_init(void)
{
    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) return rc;

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    return rc;
}

static void start_advertising(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;

    uint16_t uuid16 = 0x1805;
    fields.uuids16 = (ble_uuid16_t[]){ BLE_UUID16_INIT(uuid16) };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
}

static void on_sync(void)
{
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    start_advertising();
    ESP_LOGI(TAG, "BLE ready, advertising as \"%s\"", DEVICE_NAME);
}

static void on_reset(int reason)
{
    ESP_LOGW(TAG, "BLE host reset, reason: %d", reason);
}

static void host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_services_start(void)
{
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %d", ret);
        return;
    }

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;

    gatt_svr_init();
    ble_svc_gap_device_name_set(DEVICE_NAME);

    nimble_port_freertos_init(host_task);
}

