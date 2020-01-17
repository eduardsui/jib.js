// adapted from https://github.com/nkolban/duktape-esp32/

#include "builtins.h"
#include "builtins_adc.h"
#include "doops.h"

#include <driver/adc.h>

JS_C_FUNCTION(js_adc_config_channel_atten) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    esp_err_t errRc = adc1_config_channel_atten((adc1_channel_t)JS_GetIntParameter(ctx, 0), (adc_atten_t)JS_GetIntParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_adc_config_width) {
    JS_ParameterNumber(ctx, 0);
    esp_err_t errRc = adc1_config_width((adc_bits_width_t)JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_adc_get_voltage) {
    JS_ParameterNumber(ctx, 0);
    int value = adc1_get_voltage((adc1_channel_t)JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, value);
}

void register_adc_functions(void *main_loop, void *js_ctx) {
    register_object(js_ctx, "adc",
        "configChannelAttenuation", js_adc_config_channel_atten,
        "configWidth", js_adc_config_width,
        "getVoltage", js_adc_get_voltage,
        NULL, NULL
    );
}
