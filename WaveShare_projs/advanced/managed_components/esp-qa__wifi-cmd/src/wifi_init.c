/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"

#include "wifi_cmd.h"
#include "esp_system.h"

#ifndef APP_TAG
#define APP_TAG "WIFI"
#endif

typedef enum {
    WIFI_ACTION_INIT = 0,
    WIFI_ACTION_START,
    WIFI_ACTION_STOP,
    WIFI_ACTION_DEINIT,
    WIFI_ACTION_RESTART,
    WIFI_ACTION_STATUS,
    WIFI_ACTION_RESTORE,
} wifi_action_t;

typedef struct {
    struct arg_str *action;
    struct arg_int *espnow_enc;
    struct arg_str *storage;
    struct arg_lit *no_reboot;
    struct arg_end *end;
} wifi_init_deinit_args_t;
static wifi_init_deinit_args_t wifi_args;

#if !CONFIG_WIFI_CMD_NO_LWIP
#if CONFIG_ESP_WIFI_SOFTAP_SUPPORT
esp_netif_t *g_netif_ap = NULL;
#endif /* CONFIG_ESP_WIFI_SOFTAP_SUPPORT */
esp_netif_t *g_netif_sta = NULL;
#endif /* !CONFIG_WIFI_CMD_NO_LWIP */
int g_wifi_connect_retry_cnt = 0;
static wifi_init_config_t *s_wifi_init_cfg = NULL;

static wifi_cmd_status_t s_wifi_cmd_wifi_status = WIFI_CMD_STATUS_NONE;

static esp_err_t app_initialise_netif_and_event_loop(void)
{
    static bool s_netif_initialized = false;
    if (s_netif_initialized) {
        ESP_LOGD(APP_TAG, "netif and event_loop already initialise.");
        return ESP_OK;
    }
    esp_event_loop_create_default();
#if !CONFIG_WIFI_CMD_NO_LWIP
    esp_netif_init();
#if CONFIG_ESP_WIFI_SOFTAP_SUPPORT
    g_netif_ap = esp_netif_create_default_wifi_ap();
    assert(g_netif_ap);
#endif
    g_netif_sta = esp_netif_create_default_wifi_sta();
    assert(g_netif_sta);
#endif /* !CONFIG_WIFI_CMD_NO_LWIP */

    s_netif_initialized = true;
    return ESP_OK;
}

/* TODO: merge this function into wifi_cmd_initialize_wifi */
static esp_err_t wifi_cmd_init_wifi_and_handlers(wifi_init_config_t *cfg)
{
    app_initialise_netif_and_event_loop();

    esp_err_t ret = esp_wifi_init(cfg);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_cmd_handler_sta_disconnected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifi_cmd_handler_sta_connected, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_cmd_handler_scan_done, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_BEACON_TIMEOUT, &wifi_cmd_handler_sta_beacon_timeout, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_cmd_handler_sta_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_cmd_handler_ap_stadisconnected, NULL));
#if WIFI_CMD_IPV6_ENABLED
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &wifi_cmd_handler_sta_got_ipv6, NULL));
#endif
#if WIFI_CMD_ITWT_ENABLED
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_ITWT_SETUP, &wifi_cmd_handler_itwt_setup, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_ITWT_TEARDOWN, &wifi_cmd_handler_itwt_teardown, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_ITWT_SUSPEND, &wifi_cmd_handler_itwt_suspend, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_ITWT_PROBE, &wifi_cmd_handler_itwt_probe, NULL));
#endif /* WIFI_CMD_ITWT_ENABLED */

#if CONFIG_WIFI_CMD_DEFAULT_COUNTRY_CN
    /* Only set country once during first initialize wifi */
    static bool country_code_has_set = false;
    if (country_code_has_set == false) {
        wifi_country_t country = {
            .cc = "CN",
            .schan = 1,
            .nchan = 13,
            .policy = 0
        };
        esp_wifi_set_country(&country);
        country_code_has_set = true;
    }
#endif
    return ret;
}

static esp_err_t app_wifi_deinit(void)
{
    esp_err_t ret = esp_wifi_deinit();

    /* Add a delay here to see if there is any event triggered during wifi deinit */
    vTaskDelay(1);

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_cmd_handler_sta_disconnected));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifi_cmd_handler_sta_connected));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_cmd_handler_scan_done));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_BEACON_TIMEOUT, &wifi_cmd_handler_sta_beacon_timeout));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_cmd_handler_sta_got_ip));
#if WIFI_CMD_IPV6_ENABLED
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &wifi_cmd_handler_sta_got_ipv6));
#endif
    return ret;
}

static void alloc_wifi_init_cfg_default(void)
{
    if (s_wifi_init_cfg == NULL) {
        s_wifi_init_cfg = (wifi_init_config_t *)malloc(sizeof(wifi_init_config_t));
        if (s_wifi_init_cfg == NULL) {
            ESP_LOGE(APP_TAG, "Failed to allocate memory for wifi_init_config_t");
            abort();
        }
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        memcpy(s_wifi_init_cfg, &cfg, sizeof(wifi_init_config_t));
    }
}

esp_err_t wifi_cmd_wifi_init(wifi_cmd_initialize_cfg_t *config)
{
    if (s_wifi_cmd_wifi_status >= WIFI_CMD_STATUS_INIT) {
        ESP_LOGD(APP_TAG, "WiFi already initialized.");
        return ESP_OK;
    }
    alloc_wifi_init_cfg_default();

    if (config != NULL) {
        if (config->magic == WIFI_CMD_INIT_MAGIC) {
            *s_wifi_init_cfg = config->wifi_init_cfg;
            ESP_ERROR_CHECK(wifi_cmd_init_wifi_and_handlers(s_wifi_init_cfg));
        } else if (config->magic == WIFI_CMD_INIT_MAGIC_DEPRECATED_APP) {
            s_wifi_init_cfg->nvs_enable = config->storage == WIFI_STORAGE_FLASH ? 1 : 0;
            ESP_ERROR_CHECK(wifi_cmd_init_wifi_and_handlers(s_wifi_init_cfg));
            ESP_ERROR_CHECK(esp_wifi_set_ps(config->ps_type));
        } else {
            ESP_LOGW(APP_TAG, "wifi_cmd_initialize_wifi: ignore parameter config because magic is not set, please init with WIFI_CMD_INITIALIZE_CONFIG_DEFAULT");
            ESP_ERROR_CHECK(wifi_cmd_init_wifi_and_handlers(s_wifi_init_cfg));
        }
    } else {
        ESP_ERROR_CHECK(wifi_cmd_init_wifi_and_handlers(s_wifi_init_cfg));
    }

#if CONFIG_ESP_WIFI_ENABLED /* set 11b rate is not supported for remote wifi */
    if (config && config->disable_11b_rate) {
#if CONFIG_ESP_WIFI_SOFTAP_SUPPORT
        ESP_ERROR_CHECK(esp_wifi_config_11b_rate(WIFI_IF_AP, true));
#endif
        ESP_ERROR_CHECK(esp_wifi_config_11b_rate(WIFI_IF_STA, true));
    }
#endif /* CONFIG_ESP_WIFI_ENABLED */

#if CONFIG_WIFI_CMD_INIT_TO_STATION
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
#endif
    s_wifi_cmd_wifi_status = WIFI_CMD_STATUS_INIT;
    return ESP_OK;
}

/**
* @brief esp_wifi_start() and set wifi-cmd status
*/
esp_err_t wifi_cmd_wifi_start(void)
{
    if (s_wifi_cmd_wifi_status >= WIFI_CMD_STATUS_STARTED) {
        ESP_LOGD(APP_TAG, "WiFi already started.");
        return ESP_OK;
    }
    esp_err_t ret = esp_wifi_start();
    if (ret == ESP_OK) {
        s_wifi_cmd_wifi_status = WIFI_CMD_STATUS_STARTED;
    } else {
        ESP_LOGE(APP_TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
    }
    return ret;
}

/**
  * Used in app_main for simple wifi initialize.
  */
void wifi_cmd_initialize_wifi(wifi_cmd_initialize_cfg_t *config)
{
    ESP_ERROR_CHECK(wifi_cmd_wifi_init(config));
    ESP_ERROR_CHECK(wifi_cmd_wifi_start());
}

esp_err_t wifi_cmd_do_action(wifi_action_t action)
{
    esp_err_t ret = ESP_OK;
    switch (action) {
    case WIFI_ACTION_RESTART:
    /* fall through */
    case WIFI_ACTION_STOP:
        ret = esp_wifi_stop();
        WIFI_ERR_CHECK_LOG(ret, "esp_wifi_stop");
        if (s_wifi_cmd_wifi_status >= WIFI_CMD_STATUS_INIT) {
            s_wifi_cmd_wifi_status = WIFI_CMD_STATUS_INIT;
        }
        if (action != WIFI_ACTION_RESTART) {
            break;
        }
    /* fall through */
    case WIFI_ACTION_DEINIT:
        ret = app_wifi_deinit();
        WIFI_ERR_CHECK_LOG(ret, "app_wifi_deinit");
        s_wifi_cmd_wifi_status = WIFI_CMD_STATUS_NONE;
        if (action != WIFI_ACTION_RESTART) {
            break;
        }
    /* fall through */
    case WIFI_ACTION_INIT:
        ret = wifi_cmd_init_wifi_and_handlers(s_wifi_init_cfg);
        WIFI_ERR_CHECK_LOG(ret, "wifi_cmd_init_wifi_and_handlers");
        if (s_wifi_cmd_wifi_status <= WIFI_CMD_STATUS_INIT) {
            s_wifi_cmd_wifi_status = WIFI_CMD_STATUS_INIT;
        }
        if (action != WIFI_ACTION_RESTART) {
            break;
        }
    /* fall through */
    case WIFI_ACTION_START:
        ret = esp_wifi_start();
        WIFI_ERR_CHECK_LOG(ret, "esp_wifi_start");
        s_wifi_cmd_wifi_status = WIFI_CMD_STATUS_STARTED;
        break;
    default:
        break;
    }
    return ret;
}

static int cmd_do_set_wifi_init_deinit(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &wifi_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_args.end, argv[0]);
        return 1;
    }

    wifi_action_t action = 0;
    esp_err_t ret = ESP_OK;
    if (strcmp(wifi_args.action->sval[0], "init") == 0) {
        action = WIFI_ACTION_INIT;
    } else if (strcmp(wifi_args.action->sval[0], "deinit") == 0) {
        action = WIFI_ACTION_DEINIT;
    } else if (strcmp(wifi_args.action->sval[0], "start") == 0) {
        action = WIFI_ACTION_START;
    } else if (strcmp(wifi_args.action->sval[0], "stop") == 0) {
        action = WIFI_ACTION_STOP;
    } else if (strcmp(wifi_args.action->sval[0], "restart") == 0) {
        action = WIFI_ACTION_RESTART;
    } else if (strcmp(wifi_args.action->sval[0], "restore") == 0) {
        action = WIFI_ACTION_RESTORE;
    } else if (strcmp(wifi_args.action->sval[0], "status") == 0) {
        action = WIFI_ACTION_STATUS;
    } else {
        ESP_LOGE(APP_TAG, "invaild input action!");
        return 1;
    }
    /* wifi status */
    if (action == WIFI_ACTION_STATUS) {
        ESP_LOGI(APP_TAG, "wifi status: %d", s_wifi_cmd_wifi_status);
        wifi_cmd_query_wifi_info();
        return 0;
    }

    if (wifi_args.espnow_enc->count > 0) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 4)
        s_wifi_init_cfg->espnow_max_encrypt_num = wifi_args.espnow_enc->ival[0];
        ESP_LOGI(APP_TAG, "set global wifi config espnow encrypt number: %d", s_wifi_init_cfg->espnow_max_encrypt_num);
#else
        ESP_LOGW(APP_TAG, "espnow enc number not supported <= v4.3");
#endif
    }
    if (wifi_args.storage->count > 0) {
        if (strcmp(wifi_args.storage->sval[0], "flash") == 0) {
            s_wifi_init_cfg->nvs_enable = 1;
        } else if (strcmp(wifi_args.storage->sval[0], "ram") == 0) {
            s_wifi_init_cfg->nvs_enable = 0;
        } else {
            ESP_LOGE(APP_TAG, "invaild input storage, ignore!");
        }
    }

    if (action == WIFI_ACTION_RESTORE) {
        if (wifi_args.no_reboot->count > 0) {
            ret = esp_wifi_restore();
            if (ret != ESP_OK) {
                ESP_LOGE(APP_TAG, "esp_wifi_restore failed");
                return 1;
            }
            ESP_LOGI(APP_TAG, "esp_wifi_restore,OK");
        } else {
            ret = esp_wifi_restore();
            if (ret != ESP_OK) {
                ESP_LOGE(APP_TAG, "esp_wifi_restore failed");
                return 1;
            }
            ESP_LOGI(APP_TAG, "esp_wifi_restore,OK");
            esp_restart();
        }
        return 0;
    }

    /* TODO: add parameter init_cfg, do not update global config if operation failed. */
    esp_err_t err = wifi_cmd_do_action(action);

    LOG_WIFI_CMD_DONE(err, "WIFI");
    return 0;
}

void wifi_cmd_register_wifi_init_deinit(void)
{
    /* initialize s_wifi_init_cfg */
    if (s_wifi_init_cfg == NULL) {
        s_wifi_init_cfg = (wifi_init_config_t *)malloc(sizeof(wifi_init_config_t));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        memcpy(s_wifi_init_cfg, &cfg, sizeof(wifi_init_config_t));
    }

    wifi_args.action = arg_str1(NULL, NULL, "<action>", "init; deinit; start; stop; restart; status; restore;");
    wifi_args.espnow_enc = arg_int0(NULL, "espnow_enc", "<int>", "espnow encryption number (idf>=4.4), only for init and restart");
    wifi_args.storage = arg_str0(NULL, "storage", "<str>", "set wifi storage 'flash' or 'ram' during init");
    wifi_args.no_reboot = arg_lit0(NULL, "no_reboot", "restore without reboot");
    wifi_args.end = arg_end(2);
    const esp_console_cmd_t wifi_base_cmd = {
        .command = "wifi",
        .help = "Wi-Fi base operations: init, start, etc.",
        .hint = NULL,
        .func = &cmd_do_set_wifi_init_deinit,
        .argtable = &wifi_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_base_cmd));
}
