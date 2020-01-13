#include "builtins.h"
#include "builtins_esp32.h"

#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_vfs_dev.h>
#include <driver/uart.h>
#include <soc/rtc_wdt.h>
#include "misc/esp32/lora.h"

JS_C_FUNCTION(js_lora_set_pins) {
    JS_ParameterNumber(ctx, 0); // cs
    JS_ParameterNumber(ctx, 1); // rst
    JS_ParameterNumber(ctx, 2); // miso
    JS_ParameterNumber(ctx, 3); // mosi
    JS_ParameterNumber(ctx, 4); // sck

    lora_set_pins(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), JS_GetIntParameter(ctx, 3), JS_GetIntParameter(ctx, 4));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_init) {
    lora_init();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_reset) {
    lora_reset();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_explicit_header_mode) {
    lora_explicit_header_mode();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_implicit_header_mode) {
    JS_ParameterNumber(ctx, 0);
    lora_implicit_header_mode(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_idle) {
    lora_idle();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_sleep) {
    lora_sleep();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_receive) {
    lora_receive();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_set_tx_power) {
    JS_ParameterNumber(ctx, 0);
    lora_set_tx_power(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_set_frequency) {
    JS_ParameterNumber(ctx, 0);
    lora_set_frequency(JS_GetNumberParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_set_spreading_factor) {
    JS_ParameterNumber(ctx, 0);
    lora_set_spreading_factor(JS_GetNumberParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_set_bandwidth) {
    JS_ParameterNumber(ctx, 0);
    lora_set_bandwidth(JS_GetNumberParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_set_coding_rate) {
    JS_ParameterNumber(ctx, 0);
    lora_set_coding_rate(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_set_preamble_length) {
    JS_ParameterNumber(ctx, 0);
    lora_set_preamble_length(JS_GetNumberParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_set_sync_word) {
    JS_ParameterNumber(ctx, 0);
    lora_set_sync_word(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_enable_crc) {
    int enable = 1;
    if (JS_ParameterCount(ctx) > 1) {
        JS_ParameterBoolean(ctx, 0);

        enable = JS_GetBooleanParameter(ctx, 0);
    }
    if (enable)
        lora_enable_crc();
    else
        lora_disable_crc();

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_send_packet) {
    void *buf;
    js_size_t sz;

    buf = JS_GetBufferParameter(ctx, 0, &sz);

    if ((buf) && (sz > 0))
        lora_send_packet((uint8_t *)buf, sz);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_receive_packet) {
    // max 256 bytes packet (222 actually)
#ifdef WITH_DUKTAPE
    unsigned char *buf = (unsigned char *)duk_push_fixed_buffer(ctx, 0x100);
    if (!buf)
        JS_RETURN_NOTHING(ctx);

    int err = lora_receive_packet(buf, 0x100);
    if (err > 0) {
        duk_push_buffer_object(ctx, -1, 0, err, DUK_BUFOBJ_NODEJS_BUFFER);
        return 1;
    }
    duk_pop(ctx);
#else
    unsigned char buf[0x100];
    int err = lora_receive_packet(buf, sizeof(buf));
    if (err > 0)
        return JS_NewArrayBufferCopy(ctx, (const uint8_t *)buf, err);
#endif

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_lora_received) {
    int received = lora_received();
    JS_RETURN_NUMBER(ctx, received);
}

JS_C_FUNCTION(js_lora_packet_rssi) {
    int rssi = lora_packet_rssi();
    JS_RETURN_NUMBER(ctx, rssi);
}

JS_C_FUNCTION(js_lora_packet_snr) {
    float snr = lora_packet_snr();
    JS_RETURN_NUMBER(ctx, snr);
}

JS_C_FUNCTION(js_lora_close) {
    lora_close();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_esp_sleep_enable_timer_wakeup) {
    JS_ParameterNumber(ctx, 0);
    esp_sleep_enable_timer_wakeup(JS_GetNumberParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_esp_light_sleep_start) {
    esp_light_sleep_start();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_esp_deep_sleep_start) {
    esp_deep_sleep_start();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_esp_deep_sleep) {
    JS_ParameterNumber(ctx, 0);
    esp_deep_sleep(JS_GetNumberParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_esp_sleep_enable_touchpad_wakeup) {
    esp_err_t err = esp_sleep_enable_touchpad_wakeup();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_sleep_enable_ext0_wakeup) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 0);
    esp_err_t err = esp_sleep_enable_ext0_wakeup(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_sleep_enable_ext1_wakeup) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 0);
    esp_err_t err = esp_sleep_enable_ext1_wakeup((uint64_t)JS_GetNumberParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_sleep_enable_gpio_wakeup) {
    esp_err_t err = esp_sleep_enable_gpio_wakeup();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_sleep_enable_uart_wakeup) {
    JS_ParameterNumber(ctx, 0);
    esp_err_t err = esp_sleep_enable_uart_wakeup(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_sleep_enable_ulp_wakeup) {
    esp_err_t err = esp_sleep_enable_ulp_wakeup();
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_wake_deep_sleep) {
    esp_wake_deep_sleep();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_esp_restart) {
    esp_restart();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_esp_get_free_heap_size) {
    uint32_t size = esp_get_free_heap_size();
    JS_RETURN_NUMBER(ctx, size);
}

JS_C_FUNCTION(js_esp_get_minimum_free_heap_size) {
    uint32_t size = esp_get_minimum_free_heap_size();
    JS_RETURN_NUMBER(ctx, size);
}

JS_C_FUNCTION(js_esp_random) {
    uint32_t rand = esp_random();
    JS_RETURN_NUMBER(ctx, rand);
}

JS_C_FUNCTION(js_esp_wdt) {
    JS_ParameterBoolean(ctx, 0);

    if (JS_GetBooleanParameter(ctx, 0)) {
        // rtc_wdt_set_length_of_reset_signal(RTC_WDT_SYS_RESET_SIG, RTC_WDT_LENGTH_3_2us);
        // rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
        // rtc_wdt_set_time(RTC_WDT_STAGE0, 7000);
        rtc_wdt_enable();
        rtc_wdt_protect_on();
    } else {
        rtc_wdt_protect_off();
        rtc_wdt_disable();
    }

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_uart_driver_install) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    esp_err_t err = uart_driver_install(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), 0, NULL, 0);
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_esp_vfs_dev_uart_use_driver) {
    JS_ParameterNumber(ctx, 0);
    esp_vfs_dev_uart_use_driver(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

void register_esp32_functions(void *main_loop, void *js_ctx) {
    JS_CONTEXT ctx = (JS_CONTEXT )js_ctx;

    rtc_wdt_protect_off();
    rtc_wdt_disable();

    register_object(ctx, "lora",
        "setPins", js_lora_set_pins,
        "init", js_lora_init,
        "reset", js_lora_reset,
        "explicitHeaderMode", js_lora_explicit_header_mode,
        "implicitHeaderMode", js_lora_implicit_header_mode,
        "idle", js_lora_idle,
        "sleep", js_lora_sleep,
        "receive", js_lora_receive,
        "setTxPower", js_lora_set_tx_power,
        "setFrequency", js_lora_set_frequency,
        "setSpreadingFactor", js_lora_set_spreading_factor,
        "setBandwidth", js_lora_set_bandwidth,
        "setCodingRate", js_lora_set_coding_rate,
        "setPreambleLength", js_lora_set_preamble_length,
        "setSyncWord", js_lora_set_sync_word,
        "enableCrc", js_lora_enable_crc,
        "send", js_lora_send_packet,
        "receive", js_lora_receive_packet,
        "received", js_lora_received,
        "rssi", js_lora_packet_rssi,
        "snr", js_lora_packet_snr,
        "close", js_lora_close,
        NULL, NULL
    );

    register_object(ctx, "esp",
        "sleepEnableTimerWakeup", js_esp_sleep_enable_timer_wakeup,
        "lightSleepStart", js_esp_light_sleep_start,
        "deepSleepStart", js_esp_deep_sleep_start,
        "deepSleep", js_esp_deep_sleep,
        "enableTouchpadWakeup", js_esp_sleep_enable_touchpad_wakeup,
        "enableExt0Wakeup", js_esp_sleep_enable_ext0_wakeup,
        "enableExt1Wakeup", js_esp_sleep_enable_ext1_wakeup,
        "enableGpioWakeup", js_esp_sleep_enable_gpio_wakeup,
        "enableUartWakeup", js_esp_sleep_enable_uart_wakeup,
        "enableUlpWakeup", js_esp_sleep_enable_ulp_wakeup,
        "wake", js_esp_wake_deep_sleep,
        "restart", js_esp_restart,
        "freeHeapSize", js_esp_get_free_heap_size,
        "freeMinimumHeapSize", js_esp_get_minimum_free_heap_size,
        "random", js_esp_random,
        "enableWatchdog", js_esp_wdt,
        "random", js_esp_random,
        "uartDriverInstall", js_uart_driver_install,
        "devUartUseDriver", js_esp_vfs_dev_uart_use_driver,
        NULL, NULL
    );
}
