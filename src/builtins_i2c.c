// adapted from https://github.com/nkolban/duktape-esp32/

#include "builtins.h"
#include "builtins_i2c.h"
#include "doops.h"

#include <driver/i2c.h>

JS_C_FUNCTION(js_i2c_driver_install) {
    JS_ParameterObject(ctx, 0);

    i2c_mode_t mode = I2C_MODE_MASTER;
    i2c_port_t port = I2C_NUM_0;

    js_object_type obj = JS_GetObjectParameter(ctx, 0);
#ifdef WITH_DUKTAPE
    if (duk_get_prop_string(ctx, obj, "port") == 1)
        port = duk_get_int(ctx, -1);
    duk_pop(ctx);

    if (duk_get_prop_string(ctx, obj, "mode") == 1)
        mode = duk_get_int(ctx, -1);
    duk_pop(ctx);
#else
    js_object_type val = JS_GetPropertyStr(ctx, obj, "port");
    if (JS_IsNumber(val))
        port = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "mode");
    if (JS_IsNumber(val))
        mode = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);
#endif
    if ((port != I2C_NUM_0) && (port != I2C_NUM_1))
        JS_RETURN_NUMBER(ctx, -1);

    if ((mode != I2C_MODE_MASTER) && (mode != I2C_MODE_SLAVE))
        JS_RETURN_NUMBER(ctx, -1);

    esp_err_t errRc = i2c_driver_install(port, mode, 0, 0, 0);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_i2c_cmd_link_create) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();   
    JS_RETURN_POINTER(ctx, cmd_handle);
}

JS_C_FUNCTION(js_i2c_cmd_link_delete) {
    JS_ParameterPointer(ctx, 0);
    i2c_cmd_handle_t cmd_handle = (i2c_cmd_handle_t)JS_GetPointerParameter(ctx, 0);
    i2c_cmd_link_delete(cmd_handle);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_i2c_master_cmd_begin) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterPointer(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    i2c_port_t i2c_port = JS_GetIntParameter(ctx, 0);
    i2c_cmd_handle_t cmd_handle = (i2c_cmd_handle_t)JS_GetPointerParameter(ctx, 1);
    int delay = JS_GetIntParameter(ctx, 2);
    esp_err_t errRc = i2c_master_cmd_begin(i2c_port, cmd_handle, delay/portTICK_PERIOD_MS);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_i2c_master_read) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterBoolean(ctx, 2);
    i2c_cmd_handle_t cmd_handle = (i2c_cmd_handle_t)JS_GetPointerParameter(ctx, 0);
    int ack = JS_GetBooleanParameter(ctx, 2);
    js_size_t data_len;
    uint8_t *data = (uint8_t *)JS_GetBufferParameter(ctx, 1, &data_len);
    esp_err_t errRc = i2c_master_read(cmd_handle, data, data_len, !ack);
    JS_RETURN_NUMBER(ctx, errRc);
}


JS_C_FUNCTION(js_i2c_master_start) {
    JS_ParameterPointer(ctx, 0);
    i2c_cmd_handle_t cmd_handle = (i2c_cmd_handle_t)JS_GetPointerParameter(ctx, 0);
    esp_err_t errRc = i2c_master_start(cmd_handle);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_i2c_master_stop) {
    JS_ParameterPointer(ctx, 0);
    i2c_cmd_handle_t cmd_handle = (i2c_cmd_handle_t)JS_GetPointerParameter(ctx, 0);
    esp_err_t errRc = i2c_master_stop(cmd_handle);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_i2c_master_write) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterBoolean(ctx, 2);

    i2c_cmd_handle_t cmd_handle = (i2c_cmd_handle_t)JS_GetPointerParameter(ctx, 0);

    js_size_t data_len;
    uint8_t *data = (uint8_t *)JS_GetBufferParameter(ctx, 1, &data_len);
    if (JS_ParameterCount(ctx) > 3) {
        JS_ParameterNumber(ctx, 3);

        js_size_t len = JS_GetIntParameter(ctx, 3);
        if ((len > 0) && (len < data_len))
            data_len = len;
    }
    int ack_en = JS_GetBooleanParameter(ctx, 2);

    esp_err_t errRc = i2c_master_write(cmd_handle, data, data_len, ack_en);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_i2c_master_write_byte) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterBoolean(ctx, 2);

    i2c_cmd_handle_t cmd_handle = (i2c_cmd_handle_t)JS_GetPointerParameter(ctx, 0);
    uint8_t data = (uint8_t)JS_GetIntParameter(ctx, 1);
    int ack_en = JS_GetBooleanParameter(ctx, 2);

    esp_err_t errRc = i2c_master_write_byte(cmd_handle, data, ack_en);
    JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_i2c_param_config) {
    JS_ParameterObject(ctx, 0);

    i2c_mode_t mode = I2C_MODE_MASTER;
    i2c_port_t port = I2C_NUM_0;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t master_clk_speed = 100000;

    js_object_type obj = JS_GetObjectParameter(ctx, 0);
#ifdef WITH_DUKTAPE
    // port
    if (duk_get_prop_string(ctx, obj, "port") == 1)
        port = duk_get_int(ctx, -1);
    duk_pop(ctx);

    // mode
    if (duk_get_prop_string(ctx, obj, "mode") == 1)
        mode = duk_get_int(ctx, -1);

    duk_pop(ctx);

    // sda_pin
    if (duk_get_prop_string(ctx, obj, "sda_pin") != 1)
        JS_RETURN_NUMBER(ctx, -1);

    sda_pin = duk_get_int(ctx, -1);
    duk_pop(ctx);

    // scl_pin
    if (duk_get_prop_string(ctx, obj, "scl_pin") != 1)
        JS_RETURN_NUMBER(ctx, -1);

    scl_pin = duk_get_int(ctx, -1);
    duk_pop(ctx);

    if (mode == I2C_MODE_MASTER) {
        if (duk_get_prop_string(ctx, obj, "master_clk_speed") == 1)
            master_clk_speed = duk_get_int(ctx, -1);
        duk_pop(ctx);
    }
#else
    js_object_type val = JS_GetPropertyStr(ctx, obj, "port");
    if (JS_IsNumber(val))
        port = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "mode");
    if (JS_IsNumber(val))
        mode = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "sda_pin");
    if (JS_IsNumber(val)) {
        sda_pin = _JS_GetIntParameter(ctx, val);
    } else {
        JS_FreeValue(ctx, val);
        JS_RETURN_NUMBER(ctx, -1);
    }
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "scl_pin");
    if (JS_IsNumber(val)) {
        scl_pin = _JS_GetIntParameter(ctx, val);
    } else {
        JS_FreeValue(ctx, val);
        JS_RETURN_NUMBER(ctx, -1);
    }
    JS_FreeValue(ctx, val);

    if (mode == I2C_MODE_MASTER) {
        val = JS_GetPropertyStr(ctx, obj, "master_clk_speed");
        if (JS_IsNumber(val))
            master_clk_speed = _JS_GetIntParameter(ctx, val);
        JS_FreeValue(ctx, val);
    }
#endif
    if ((port != I2C_NUM_0) && (port != I2C_NUM_1))
        JS_RETURN_NUMBER(ctx, -1);
    if ((mode != I2C_MODE_MASTER) && (mode != I2C_MODE_SLAVE))
        JS_RETURN_NUMBER(ctx, -1);

    i2c_config_t conf;
    conf.mode = mode;
    conf.sda_io_num = sda_pin;
    conf.scl_io_num = scl_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    if (mode == I2C_MODE_MASTER)
        conf.master.clk_speed = master_clk_speed;

    esp_err_t errRc = i2c_param_config(port, &conf);
    JS_RETURN_NUMBER(ctx, errRc);
}

void register_i2c_functions(void *main_loop, void *js_ctx) {
    register_object(js_ctx, "i2c",
        "cmdLinkCreate", js_i2c_cmd_link_create,
        "cmdLinkDelete", js_i2c_cmd_link_delete,
        "driverInstall", js_i2c_driver_install,
        "masterCmdBegin", js_i2c_master_cmd_begin,
        "masterRead", js_i2c_master_read,
        "masterStart", js_i2c_master_start,
        "masterStop", js_i2c_master_stop,
        "masterWrite", js_i2c_master_write,
        "masterWriteByte", js_i2c_master_write_byte,
        "paramConfig", js_i2c_param_config,
        NULL, NULL
    );
    JS_EvalSimple(js_ctx, "i2c.constants = {I2C_NUM_0:0,I2C_NUM_1:1,I2C_MODE_MASTER:1,I2C_MODE_SLAVE:0,I2C_MASTER_READ:1,I2C_MASTER_WRITE:0};");
}
