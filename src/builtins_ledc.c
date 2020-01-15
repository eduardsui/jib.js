// adapted from https://github.com/nkolban/duktape-esp32/

#include "builtins.h"
#include "builtins_spi.h"
#include "doops.h"

#include <driver/ledc.h>

JS_C_FUNCTION(js_ledc_channel_config) {
    int channel = 0;
    int duty = 0;
    int gpio = 0;
    int timer = 0;

    js_object_type obj = JS_GetObjectParameter(ctx, 0);
#ifdef WITH_DUKTAPE
    if (duk_get_prop_string(ctx, obj, "gpio") == 1) {
        gpio = duk_get_int(ctx, -1);
    } else {
        JS_Error(ctx, "gpio not specified");
    }
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, obj, "duty") == 1) {
        duty = duk_get_int(ctx, -1);
    } else {
        JS_Error(ctx, "duty not specified");
    }
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, obj, "channel") == 1) {
        channel = duk_get_int(ctx, -1);
    } else {
        JS_Error(ctx, "channel not specified");
    }
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, obj, "timer") == 1) {
        timer = duk_get_int(ctx, -1);
    } else {
        JS_Error(ctx, "timer not specified");
    }
    duk_pop(ctx);
#else
    js_object_type val = JS_GetPropertyStr(ctx, obj, "gpio");
    if (JS_IsNumber(val)) {
        gpio = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    } else {
        JS_FreeValue(ctx, val);
        JS_Error(ctx, "gpio not specified");
    }

    val = JS_GetPropertyStr(ctx, obj, "duty");
    if (JS_IsNumber(val)) {
        duty = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    } else {
        JS_Error(ctx, "duty not specified");
        JS_FreeValue(ctx, val);
    }

    val = JS_GetPropertyStr(ctx, obj, "channel");
    if (JS_IsNumber(val)) {
        channel = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    } else {
        JS_FreeValue(ctx, val);
        JS_Error(ctx, "channel not specified");
    }

    val = JS_GetPropertyStr(ctx, obj, "timer");
    if (JS_IsNumber(val)) {
        timer = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    } else {
        JS_FreeValue(ctx, val);
        JS_Error(ctx, "timer not specified");
    }
#endif
    if ((channel < 0) || (channel > 7))
        JS_Error(ctx, "channel invalid");

    if ((timer < 0) || (timer > 3))
        JS_Error(ctx, "timer invalid");

    ledc_channel_config_t ledc_conf = {
       .channel    = channel,
       .duty       = duty,
       .gpio_num   = gpio,
       .intr_type  = LEDC_INTR_DISABLE,
       .speed_mode = LEDC_HIGH_SPEED_MODE,
       .timer_sel  = timer
    };
    esp_err_t errRc = ledc_channel_config(&ledc_conf);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_ledc_set_duty) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    esp_err_t errRc;
    ledc_channel_t channel;
    uint32_t duty;
    channel = JS_GetIntParameter(ctx, 0);
    if (channel < 0 || channel >= LEDC_CHANNEL_MAX) {
        JS_Error(ctx, "invalid channel");
    }
    duty = JS_GetIntParameter(ctx, 1);
    errRc = ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel, duty);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_ledc_timer_config) {
    JS_ParameterObject(ctx, 0);

    int freq_hz = 1000;
    int bit_size = 10;
    int timer = 0;

    js_object_type obj = JS_GetObjectParameter(ctx, 0);
#ifdef WITH_DUKTAPE
    if (duk_get_prop_string(ctx, obj, "freq") == 1) {
        freq_hz = duk_get_int(ctx, -1);
    } else {
        JS_Error(ctx, "freq not specified");
    }
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, obj, "bitSize") == 1) {
        bit_size = duk_get_int(ctx, -1);
    } else {
        JS_Error(ctx, "bitSize not specified");
    }
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, obj, "timer") == 1) {
        timer = duk_get_int(ctx, -1);
    } else {
        JS_Error(ctx, "timer not specified");
    }
    duk_pop(ctx);
#else
    js_object_type val = JS_GetPropertyStr(ctx, obj, "freq");
    if (JS_IsNumber(val)) {
        freq_hz = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    } else {
        JS_FreeValue(ctx, val);
        JS_Error(ctx, "freq not specified");
    }

    val = JS_GetPropertyStr(ctx, obj, "bitSize");
    if (JS_IsNumber(val)) {
        bit_size = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    } else {
        JS_Error(ctx, "bitSize not specified");
        JS_FreeValue(ctx, val);
    }

    val = JS_GetPropertyStr(ctx, obj, "timer");
    if (JS_IsNumber(val)) {
        timer = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    } else {
        JS_FreeValue(ctx, val);
        JS_Error(ctx, "timer not specified");
    }
#endif
    if ((bit_size < 10) || (bit_size > 15))
        JS_Error(ctx, "bitSize invalid");
    if ((timer < 0) || (timer > 3))
        JS_Error(ctx, "timer invalid");

    ledc_timer_config_t timer_conf = {
       .bit_num    = bit_size,
       .freq_hz    = freq_hz,
       .speed_mode = LEDC_HIGH_SPEED_MODE,
       .timer_num  = timer,
    };

    esp_err_t errRc = ledc_timer_config(&timer_conf);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_ledc_update_duty) {
    JS_ParameterNumber(ctx, 0);
    esp_err_t errRc;
    ledc_channel_t channel;
    channel = JS_GetIntParameter(ctx, 0);
    if (channel < 0 || channel >= LEDC_CHANNEL_MAX) {
        JS_Error(ctx, "invalid channel");
    }
    errRc = ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel);
    JS_RETURN_NUMBER(ctx, errRc);
}

void register_ledc_functions(void *main_loop, void *js_ctx) {
    register_object(js_ctx, "ledc",
        "configureChannel", js_ledc_channel_config,
        "setDuty", js_ledc_set_duty,
        "configureTimer", js_ledc_timer_config,
        "updateDuty", js_ledc_update_duty,
        NULL, NULL
    );
}
