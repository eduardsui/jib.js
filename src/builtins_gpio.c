#include "builtins.h"
#include "builtins_gpio.h"
#include "doops.h"

#include <driver/gpio.h>

static unsigned char gpio_is_installed = 0;

static int gpio_js_isr_handler(struct doops_loop *loop) {
    char buf[0x40];
    snprintf(buf, sizeof(buf), "if (gpio.onisr) gpio.onisr(%i);", (int)(int32_t)loop_event_data(loop));
    JS_EvalSimple(js(), buf);
    return 1;
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    // ensure in js loop
    loop_add(js_loop(), gpio_js_isr_handler, (int)0, arg);
}

JS_C_FUNCTION(js_gpio_padSelectGpio) {
    JS_ParameterNumber(ctx, 0);
    gpio_pad_select_gpio(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_gpio_setDirection) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    int err = gpio_set_direction(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_gpio_setIntrType) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    if (!gpio_is_installed) {
        gpio_install_isr_service(0);
        gpio_is_installed = 1;
    }
    int err = gpio_set_intr_type(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    gpio_isr_handler_add(JS_GetIntParameter(ctx, 0), gpio_isr_handler, (void *)JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_gpio_setLevel) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    int err = gpio_set_level(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_gpio_getLevel) {
    JS_ParameterNumber(ctx, 0);
    int val = gpio_get_level(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, val);
}


JS_C_FUNCTION(js_gpio_isr_handler_remove) {
    JS_ParameterNumber(ctx, 0);
    int val = gpio_isr_handler_remove(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, val);
}

void register_gpio_functions(void *main_loop, void *js_ctx) {
    JS_CONTEXT ctx = (JS_CONTEXT )js_ctx;

    register_object(ctx, "gpio",
        "padSelectGpio", js_gpio_padSelectGpio,
        "setDirection", js_gpio_setDirection,
        "setLevel", js_gpio_setLevel,
        "getLevel", js_gpio_getLevel,
        "setIntrType", js_gpio_setIntrType,
        "removeIntr", js_gpio_isr_handler_remove,
        NULL, NULL
    );
    JS_EvalSimple(ctx, "gpio.constants = { GPIO_MODE_INPUT: 0, GPIO_MODE_OUTPUT: 1, GPIO_MODE_AF: 2, GPIO_MODE_ANALOG: 3, GPIO_INTR_DISABLE: 0, GPIO_INTR_POSEDGE: 1, GPIO_INTR_NEGEDGE: 2, GPIO_INTR_ANYEDGE: 3, GPIO_INTR_LOW_LEVEL: 4, GPIO_INTR_HIGH_LEVEL: 5 }");
}
