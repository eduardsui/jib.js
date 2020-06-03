#include "builtins.h"
#include "builtins_wifi.h"
#include "doops.h"

#include <esp_wifi.h>
#include <tcpip_adapter.h>
#include <esp_event_loop.h>
#include <nvs_flash.h>
#include <freertos/event_groups.h>

static unsigned char wifi_initialized = 0;
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int DISCONNECTED_BIT = BIT1;
char last_ip[0x40] = { 0 };
char last_gateway[0x40] = { 0 };
char last_netmask[0x40] = { 0 };

#define STR_HELPER(x)   #x
#define TO_STR(x)       STR_HELPER(x)

static int wifi_scan_done_handler(struct doops_loop *loop) {
    JS_EvalSimple(js(), "if (wifi.onscandone) wifi.onscandone();");
    return 1;
}

static int wifi_disconnect_handler(struct doops_loop *loop) {
    JS_EvalSimple(js(), "if (wifi.ondisconnect) wifi.ondisconnect();");
    return 1;
}

static int wifi_gotip_handler(struct doops_loop *loop) {
    JS_EvalSimple(js(), "if (wifi.onip) wifi.onip();");
    return 1;
}

static int wifi_sta_start_handler(struct doops_loop *loop) {
    JS_EvalSimple(js(), "if (wifi.onstart) wifi.onstart();");
    return 1;
}

static int wifi_sta_connected_handler(struct doops_loop *loop) {
    JS_EvalSimple(js(), "if (wifi.onclientconnected) wifi.onclientconnected();");
    return 1;
}

static int wifi_sta_disconnected_handler(struct doops_loop *loop) {
    JS_EvalSimple(js(), "if (wifi.onclientdisconnected) wifi.onclientdisconnected();");
    return 1;
}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            loop_add(js_loop(), wifi_sta_start_handler, (int)0, NULL);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            strncpy(last_ip, ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip), sizeof(last_ip));
            strncpy(last_gateway, ip4addr_ntoa(&event->event_info.got_ip.ip_info.gw), sizeof(last_gateway));
            strncpy(last_netmask, ip4addr_ntoa(&event->event_info.got_ip.ip_info.netmask), sizeof(last_netmask));
            loop_add(js_loop(), wifi_gotip_handler, (int)0, NULL);
            xEventGroupClearBits(wifi_event_group, DISCONNECTED_BIT);
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            strncpy(last_ip, ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip), sizeof(last_ip));
            loop_add(js_loop(), wifi_gotip_handler, (int)0, NULL);
            xEventGroupClearBits(wifi_event_group, DISCONNECTED_BIT);
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            loop_add(js_loop(), wifi_scan_done_handler, (int)0, NULL);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            loop_add(js_loop(), wifi_disconnect_handler, (int)0, NULL);
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            xEventGroupSetBits(wifi_event_group, DISCONNECTED_BIT);
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            loop_add(js_loop(), wifi_sta_connected_handler, (int)0, NULL);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            loop_add(js_loop(), wifi_sta_disconnected_handler, (int)0, NULL);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void ensure_wifi() {
    if (wifi_initialized)
        return;

    tcpip_adapter_init();

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_initialized = 1;
}

JS_C_FUNCTION(js_wifi_set_mode) {
    JS_ParameterNumber(ctx, 0);

    ensure_wifi();
    esp_err_t err = esp_wifi_set_mode(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_wifi_set_storage) {
    JS_ParameterNumber(ctx, 0);
    ensure_wifi();
    esp_err_t err = esp_wifi_set_storage(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_wifi_set_config) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterObject(ctx, 1);
    ensure_wifi();
    
    esp_err_t err;
#ifdef WITH_DUKTAPE
    duk_get_prop_string(ctx, 1, "ap");
    if ((JS_GetIntParameter(ctx, 0) == ESP_IF_WIFI_AP) || ((duk_is_boolean(ctx, -1)) && (duk_get_boolean(ctx, -1)))) {
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = "hello",
                .ssid_len = strlen("hello"),
                .password = "helloworld",
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            },
        };
        duk_get_prop_string(ctx, 1, "ssid");
        if (duk_is_string(ctx,-1)) {
            strncpy((char *)wifi_config.ap.ssid, duk_safe_to_string(ctx, -1), 32);
            wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
        }

        duk_get_prop_string(ctx, 1, "password");
        if (duk_is_string(ctx, -1))
            strncpy((char *)wifi_config.ap.password, duk_safe_to_string(ctx, -1), 64);

        duk_get_prop_string(ctx, 1, "authmode");
        if (duk_is_number(ctx, -1))
            wifi_config.ap.authmode = (wifi_auth_mode_t)duk_get_int(ctx, -1);

        duk_get_prop_string(ctx, 1, "max_connection");
        if (duk_is_number(ctx, -1))
            wifi_config.ap.max_connection = duk_get_int(ctx, -1);

        duk_get_prop_string(ctx, 1, "beacon_interval");
        if (duk_is_number(ctx, -1))
            wifi_config.ap.beacon_interval = duk_get_int(ctx, -1);

        duk_get_prop_string(ctx, 1, "channel");
        if (duk_is_number(ctx, -1))
            wifi_config.ap.channel = duk_get_int(ctx, -1);

        err = esp_wifi_set_config(JS_GetIntParameter(ctx, 0), &wifi_config);
    } else {
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = "",
                .password = "",
                .scan_method = WIFI_FAST_SCAN,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                .threshold.rssi = -127,
                .threshold.authmode = WIFI_AUTH_OPEN,
            },
        };

        duk_get_prop_string(ctx, 1, "ssid");
        if (duk_is_string(ctx,-1))
            strncpy((char *)wifi_config.sta.ssid, duk_safe_to_string(ctx, -1), 32);

        duk_get_prop_string(ctx, 1, "password");
        if (duk_is_string(ctx, -1))
            strncpy((char *)wifi_config.sta.password, duk_safe_to_string(ctx, -1), 64);

        duk_get_prop_string(ctx, 1, "scan_method");
        if (duk_is_number(ctx, -1))
            wifi_config.sta.scan_method = duk_get_int(ctx, -1);

        duk_get_prop_string(ctx, 1, "sort_method");
        if (duk_is_number(ctx, -1))
            wifi_config.sta.sort_method = duk_get_int(ctx, -1);

        duk_get_prop_string(ctx, 1, "threshold_rssi");
        if (duk_is_number(ctx, -1))
            wifi_config.sta.threshold.rssi = duk_get_int(ctx, -1);

        duk_get_prop_string(ctx, 1, "threshold_authmode");
        if (duk_is_number(ctx, -1))
            wifi_config.sta.threshold.authmode = duk_get_int(ctx, -1);

        err = esp_wifi_set_config(JS_GetIntParameter(ctx, 0), &wifi_config);
    }
#else
    JSValueConst ap = JS_GetPropertyStr(ctx, argv[1], "ap");
    if ((JS_GetIntParameter(ctx, 0) == ESP_IF_WIFI_AP) || ((JS_IsBool(ap)) && (_JS_GetBooleanParameter(ctx, ap)))) {
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = "hello",
                .ssid_len = strlen("hello"),
                .password = "helloworld",
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            },
        };
        JSValueConst val = JS_GetPropertyStr(ctx, argv[1], "ssid");
        if (JS_IsString(val)) {
            const char *str = JS_ToCString(ctx, val);
            strncpy((char *)wifi_config.ap.ssid, str, 32);
            wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
            JS_FreeString(ctx, str);
        }
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "password");
        if (JS_IsString(val)) {
            const char *str = JS_ToCString(ctx, val);
            strncpy((char *)wifi_config.ap.password, str, 64);
            JS_FreeString(ctx, str);
        }
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "authmode");
        if (JS_IsNumber(val))
            wifi_config.ap.authmode = (wifi_auth_mode_t)_JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "max_connection");
        if (JS_IsNumber(val))
            wifi_config.ap.max_connection = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "beacon_interval");
        if (JS_IsNumber(val))
            wifi_config.ap.beacon_interval = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "channel");
        if (JS_IsNumber(val))
            wifi_config.ap.channel = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        err = esp_wifi_set_config(JS_GetIntParameter(ctx, 0), &wifi_config);
    } else {
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = "",
                .password = "",
                .scan_method = WIFI_FAST_SCAN,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                .threshold.rssi = -127,
                .threshold.authmode = WIFI_AUTH_OPEN,
            },
        };

        JSValueConst val = JS_GetPropertyStr(ctx, argv[1], "ssid");
        if (JS_IsString(val)) {
            const char *str = JS_ToCString(ctx, val);
            strncpy((char *)wifi_config.ap.ssid, str, 32);
            wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
            JS_FreeString(ctx, str);
        }
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "password");
        if (JS_IsString(val)) {
            const char *str = JS_ToCString(ctx, val);
            strncpy((char *)wifi_config.ap.password, str, 64);
            JS_FreeString(ctx, str);
        }
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "scan_method");
        if (JS_IsNumber(val))
            wifi_config.sta.scan_method = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "sort_method");
        if (JS_IsNumber(val))
            wifi_config.sta.sort_method = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "threshold_rssi");
        if (JS_IsNumber(val))
            wifi_config.sta.threshold.rssi = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        val = JS_GetPropertyStr(ctx, argv[1], "threshold_authmode");
        if (JS_IsNumber(val))
            wifi_config.sta.threshold.authmode = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);

        err = esp_wifi_set_config(JS_GetIntParameter(ctx, 0), &wifi_config);
    }
    JS_FreeValue(ctx, ap);
#endif
    
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_wifi_start) {
    ensure_wifi();
    esp_err_t err = esp_wifi_start();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_wifi_connect) {
    ensure_wifi();
    esp_err_t err = esp_wifi_connect();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_wifi_stop) {
    esp_err_t err = esp_wifi_stop();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_wifi_disconnect) {
    esp_err_t err = esp_wifi_disconnect();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_wifi_restore) {
    ensure_wifi();
    esp_err_t err = esp_wifi_restore();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wifi_scan_start) {
    ensure_wifi();

    wifi_scan_config_t scan_config = { 0 };

    const char *ssid = NULL;
    if (JS_ParameterCount(ctx) > 0) {
        JS_ParameterString(ctx, 0);
        ssid = JS_GetStringParameter(ctx, 0);
        if ((ssid) && (!ssid[0])) {
            JS_FreeString(ctx, ssid);
            ssid = NULL;
        }
    }
    scan_config.ssid = (uint8_t *) ssid;

    esp_err_t err = esp_wifi_scan_start(&scan_config, 0);
    if (ssid) {
        JS_FreeString(ctx, ssid);
    }
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wifi_scan_stop) {
    esp_err_t err = esp_wifi_scan_stop();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wifi_stations) {
    uint16_t sta_number = 0;
    uint8_t i;
    wifi_ap_record_t *ap_list_buffer;

    esp_wifi_scan_get_ap_num(&sta_number);
    ap_list_buffer = (wifi_ap_record_t *)malloc(sta_number * sizeof(wifi_ap_record_t));
    if (ap_list_buffer == NULL)
        JS_RETURN_NOTHING(ctx);

#ifdef WITH_DUKTAPE
    duk_idx_t arr_idx = duk_push_array(ctx);
    if (esp_wifi_scan_get_ap_records(&sta_number,(wifi_ap_record_t *)ap_list_buffer) == ESP_OK) {
        for (i = 0; i < sta_number; i++) {
            duk_push_string(ctx, (const char *)ap_list_buffer[i].ssid);
            duk_put_prop_index(ctx, arr_idx, i);
        }
    }
    free(ap_list_buffer);

    return 1;
#else
    js_object_type arr = JS_NewArray(ctx);
    if (esp_wifi_scan_get_ap_records(&sta_number,(wifi_ap_record_t *)ap_list_buffer) == ESP_OK) {
        for (i = 0; i < sta_number; i++)
            JS_SetPropertyUint32(ctx, arr, (unsigned int)i, JS_NewString(ctx, (const char *)ap_list_buffer[i].ssid));
    }
    free(ap_list_buffer);

    return arr;
#endif
} 

JS_C_FUNCTION(js_esp_wifi_get_ip) {
    JS_RETURN_STRING(ctx, last_ip);
}

JS_C_FUNCTION(js_esp_wifi_get_gw) {
    JS_RETURN_STRING(ctx, last_gateway);
}

JS_C_FUNCTION(js_esp_wifi_get_netmask) {
    JS_RETURN_STRING(ctx, last_netmask);
}

JS_C_FUNCTION(js_esp_wifi_start_dhcps) {
    JS_ParameterNumber(ctx, 0);

    ensure_wifi();

    esp_err_t err = tcpip_adapter_dhcps_start(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wifi_stop_dhcps) {
    JS_ParameterNumber(ctx, 0);

    ensure_wifi();

    esp_err_t err = tcpip_adapter_dhcps_stop(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wifi_start_dhcpc) {
    JS_ParameterNumber(ctx, 0);

    ensure_wifi();

    esp_err_t err = tcpip_adapter_dhcpc_start(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wifi_stop_dhcpc) {
    JS_ParameterNumber(ctx, 0);

    ensure_wifi();

    esp_err_t err = tcpip_adapter_dhcpc_stop(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wifi_set_ip_info) {
    JS_ParameterNumber(ctx, 0);

    ensure_wifi();

    tcpip_adapter_ip_info_t ip_info;
    ip_info.ip.addr = ipaddr_addr("192.168.0.1");
    ip_info.gw.addr = ipaddr_addr("0.0.0.0");
    ip_info.netmask.addr = ipaddr_addr("255.255.255.0");

    if (JS_ParameterCount(ctx) > 1) {
        JS_ParameterString(ctx, 1);
        const char *ip = JS_GetStringParameter(ctx, 1);
        ip_info.ip.addr = ipaddr_addr(ip);
        JS_FreeString(ctx, ip);
    }

    if (JS_ParameterCount(ctx) > 2) {
        JS_ParameterString(ctx, 2);
        const char *ip = JS_GetStringParameter(ctx, 2);
        ip_info.ip.addr = ipaddr_addr(ip);
        JS_FreeString(ctx, ip);
    }

    if (JS_ParameterCount(ctx) > 3) {
        JS_ParameterString(ctx, 3);
        const char *ip = JS_GetStringParameter(ctx, 3);
        ip_info.ip.addr = ipaddr_addr(ip);
        JS_FreeString(ctx, ip);
    }

    esp_err_t err = tcpip_adapter_set_ip_info(JS_GetIntParameter(ctx, 0), &ip_info);
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_err_to_name) {
    JS_ParameterNumber(ctx, 0);
    JS_RETURN_STRING(ctx, esp_err_to_name(JS_GetIntParameter(ctx, 0)));
}

JS_C_FUNCTION(js_lazy_ap_wifi) {
    JS_ParameterObject(ctx, 0);
    
    uint8_t a[6];
    esp_efuse_mac_get_default(a);
    uint64_t mac_addr =  ((uint64_t)(a[0])) << 40 |
                ((uint64_t)(a[1])) << 32 | ( 
                    ((uint32_t)(a[2])) << 24 | 
                    ((uint32_t)(a[3])) << 16 |
                    ((uint32_t)(a[4])) << 8 |
                    ((uint32_t)(a[5]))
                );

#ifdef WITH_DUKTAPE
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "hello",
            .ssid_len = strlen("hello"),
            .password = "helloworld",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    duk_get_prop_string(ctx, 0, "ssid");
    if (duk_is_string(ctx,-1)) {
        strncpy((char *)wifi_config.ap.ssid, duk_safe_to_string(ctx, -1), 32);
        wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
    } else {
        snprintf((char *)wifi_config.ap.ssid, 32, "hello_%" PRIx64, mac_addr);
        wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
    }

    duk_get_prop_string(ctx, 0, "password");
    if (duk_is_string(ctx, -1))
        strncpy((char *)wifi_config.ap.password, duk_safe_to_string(ctx, -1), 64);

    duk_get_prop_string(ctx, 0, "authmode");
    if (duk_is_number(ctx, -1))
        wifi_config.ap.authmode = (wifi_auth_mode_t)duk_get_int(ctx, -1);

    duk_get_prop_string(ctx, 0, "max_connection");
    if (duk_is_number(ctx, -1))
        wifi_config.ap.max_connection = duk_get_int(ctx, -1);

    duk_get_prop_string(ctx, 0, "beacon_interval");
    if (duk_is_number(ctx, -1))
        wifi_config.ap.beacon_interval = duk_get_int(ctx, -1);

    duk_get_prop_string(ctx, 0, "channel");
    if (duk_is_number(ctx, -1))
        wifi_config.ap.channel = duk_get_int(ctx, -1);
#else
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "hello",
            .ssid_len = strlen("hello"),
            .password = "helloworld",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    JSValueConst val = JS_GetPropertyStr(ctx, argv[0], "ssid");
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        strncpy((char *)wifi_config.ap.ssid, str, 32);
        wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
        JS_FreeString(ctx, str);
    } else {
        snprintf((char *)wifi_config.ap.ssid, 32, "hello_%" PRIx64, mac_addr);
        wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
    }
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, argv[0], "password");
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        strncpy((char *)wifi_config.ap.password, str, 64);
        JS_FreeString(ctx, str);
    }
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, argv[0], "authmode");
    if (JS_IsNumber(val))
        wifi_config.ap.authmode = (wifi_auth_mode_t)_JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, argv[0], "max_connection");
    if (JS_IsNumber(val))
        wifi_config.ap.max_connection = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, argv[0], "beacon_interval");
    if (JS_IsNumber(val))
        wifi_config.ap.beacon_interval = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, argv[0], "channel");
    if (JS_IsNumber(val))
        wifi_config.ap.channel = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);
#endif
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (!wifi_initialized) {
        tcpip_adapter_init();

        tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);

        tcpip_adapter_ip_info_t ip_info;
        ip_info.ip.addr = ipaddr_addr("192.168.48.1");
        ip_info.gw.addr = ipaddr_addr("0.0.0.0");
        ip_info.netmask.addr = ipaddr_addr("255.255.255.0");
        ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));

        tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);

        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_initialized = 1;
    }
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lazy_sta_wifi) {
    JS_ParameterString(ctx, 0);
    JS_ParameterString(ctx, 1);

    const char *ssid = JS_GetStringParameter(ctx, 0);
    const char *pass = JS_GetStringParameter(ctx, 1);

    if (!wifi_initialized) {
        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        wifi_initialized = 1;
    }
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, 32);
    strncpy((char *)wifi_config.sta.password, pass, 64);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    JS_FreeString(ctx, ssid);
    JS_FreeString(ctx, pass);

    JS_RETURN_NOTHING(ctx);
}

void register_wifi_functions(void *main_loop, void *js_ctx) {
    JS_CONTEXT ctx = (JS_CONTEXT )js_ctx;

    register_object(ctx, "wifi",
        "setMode", js_wifi_set_mode,
        "setConfig", js_wifi_set_config,
        "setStorage", js_wifi_set_storage,
        "start", js_wifi_start,
        "stop", js_wifi_stop,
        "restore", js_wifi_restore,
        "connect", js_wifi_connect,
        "disconnect", js_wifi_disconnect,
        "startScan", js_esp_wifi_scan_start,
        "stopScan", js_esp_wifi_scan_stop,
        "stations", js_esp_wifi_stations,
        "getIp", js_esp_wifi_get_ip,
        "getGateway", js_esp_wifi_get_gw,
        "getNetmask", js_esp_wifi_get_netmask,
        "startDhcps", js_esp_wifi_start_dhcps,
        "stopDhcps", js_esp_wifi_stop_dhcps,
        "startDhcpc", js_esp_wifi_start_dhcpc,
        "stopDhcpc", js_esp_wifi_stop_dhcpc,
        "setIp", js_esp_wifi_set_ip_info,
        "strerror", js_esp_err_to_name,
        "ap", js_lazy_ap_wifi,
        "station", js_lazy_sta_wifi,
        NULL, NULL
    );

    JS_EvalSimple(ctx, "wifi.constants = { ESP_IF_WIFI_STA: 0"
                                        ", ESP_IF_WIFI_AP: 1"
                                        ", ESP_IF_ETH: 2"
                                        ", TCPIP_ADAPTER_IF_STA: 0"
                                        ", TCPIP_ADAPTER_IF_AP: 1"
                                        ", TCPIP_ADAPTER_IF_ETH: 2"
                                        ", WIFI_MODE_NULL: 0"
                                        ", WIFI_MODE_STA: 1"
                                        ", WIFI_MODE_AP: 2"
                                        ", WIFI_MODE_APSTA: 3"
                                        ", WIFI_STORAGE_FLASH: 0"
                                        ", WIFI_STORAGE_RAM: 1"
                                        ", ESP_OK:" TO_STR(ESP_OK)
                                        ", ESP_ERR_WIFI_NOT_INIT: " TO_STR(ESP_ERR_WIFI_NOT_INIT)
                                        ", ESP_ERR_INVALID_ARG: " TO_STR(ESP_ERR_INVALID_ARG)
                                        ", WIFI_AUTH_OPEN: 0"
                                        ", WIFI_AUTH_WEP: 1"
                                        ", WIFI_AUTH_WPA_PSK: 2"
                                        ", WIFI_AUTH_WPA2_PSK: 3"
                                        ", WIFI_AUTH_WPA_WPA2_PSK: 4"
                                        ", WIFI_AUTH_WPA2_ENTERPRISE: 5"
    " }");
}
