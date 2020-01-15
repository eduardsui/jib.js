// adapted from https://github.com/nkolban/duktape-esp32/

#include "builtins.h"
#include "builtins_spi.h"
#include "doops.h"

#include <driver/spi_master.h>

#define DEFAULT_HOST HSPI_HOST
#define DEFAULT_CLOCK_SPEED (10000)

JS_C_FUNCTION(js_spi_bus_add_device) {
    JS_ParameterObject(ctx, 0);

	spi_host_device_t host = DEFAULT_HOST;
	spi_device_interface_config_t dev_config;
	spi_device_handle_t *handle;

	dev_config.command_bits     = 0;
	dev_config.address_bits     = 0;
	dev_config.dummy_bits       = 0;
	dev_config.duty_cycle_pos   = 0;
	dev_config.cs_ena_posttrans = 0;
	dev_config.cs_ena_pretrans  = 0;
	dev_config.clock_speed_hz   = DEFAULT_CLOCK_SPEED;
	dev_config.spics_io_num     = -1;
	dev_config.flags            = 0;
	dev_config.queue_size       = 10;
	dev_config.pre_cb           = NULL;
	dev_config.post_cb          = NULL;
	dev_config.mode             = 0;
    

    js_object_type obj = JS_GetObjectParameter(ctx, 0);
#ifdef WITH_DUKTAPE
	// host
	if (duk_get_prop_string(ctx, obj, "host") == 1)
		host = duk_get_int(ctx, -1);
	duk_pop(ctx);

	// mode
	if (duk_get_prop_string(ctx, obj, "mode") == 1)
		dev_config.mode = duk_get_int(ctx, -1);
	duk_pop(ctx);

	// clock_speed
	if (duk_get_prop_string(ctx, obj, "clock_speed") == 1)
		dev_config.clock_speed_hz = duk_get_int(ctx, -1);
	duk_pop(ctx);

	// cs
	if (duk_get_prop_string(ctx, obj, "cs") == 1)
		dev_config.spics_io_num = duk_get_int(ctx, -1);
	duk_pop(ctx);
#else
    js_object_type val = JS_GetPropertyStr(ctx, obj, "host");
    if (JS_IsNumber(val))
        host = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "mode");
    if (JS_IsNumber(val))
        dev_config.mode = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "clock_speed");
    if (JS_IsNumber(val))
        dev_config.clock_speed_hz = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "cs");
    if (JS_IsNumber(val))
        dev_config.spics_io_num = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);
#endif

	handle = (spi_device_handle_t *)malloc(sizeof(spi_device_handle_t));
	assert(handle != NULL);

	esp_err_t errRc = spi_bus_add_device(host, &dev_config, handle);
	if (errRc != ESP_OK) {
		free(handle);
		JS_RETURN_NOTHING(ctx);
	}

	JS_RETURN_POINTER(ctx, handle);
}


JS_C_FUNCTION(js_spi_bus_free) {
	spi_host_device_t host = DEFAULT_HOST;
    if (JS_ParameterCount(ctx) > 0) {
        JS_ParameterNumber(ctx, 0);
		host = JS_GetNumberParameter(ctx, 0);
    }
	esp_err_t errRc = spi_bus_free(host);
	if (errRc != ESP_OK) {
		JS_RETURN_NOTHING(ctx);
	}
	JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_spi_bus_initialize) {
    JS_ParameterObject(ctx, 0);

	spi_host_device_t host = DEFAULT_HOST;
	spi_bus_config_t bus_config;
	int dma_chan;

	bus_config.mosi_io_num   = -1; // MOSI
	bus_config.miso_io_num   = -1; // MISO
	bus_config.sclk_io_num = -1; // CLK
	bus_config.quadwp_io_num  = -1;
	bus_config.quadhd_io_num  = -1;
	dma_chan = 1;

    js_object_type obj = JS_GetObjectParameter(ctx, 0);
#ifdef WITH_DUKTAPE
	// mosi
	if (duk_get_prop_string(ctx, obj, "mosi") == 1) {
		bus_config.mosi_io_num = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// miso
	if (duk_get_prop_string(ctx, obj, "miso") == 1) {
		bus_config.miso_io_num = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// clk
	if (duk_get_prop_string(ctx, obj, "clk") == 1) {
		bus_config.sclk_io_num = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);

	// host
	if (duk_get_prop_string(ctx, obj, "host") == 1) {
		host = duk_get_int(ctx, -1);
	}
	duk_pop(ctx);
#else
    js_object_type val = JS_GetPropertyStr(ctx, obj, "mosi");
    if (JS_IsNumber(val))
        bus_config.mosi_io_num = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "miso");
    if (JS_IsNumber(val))
        bus_config.miso_io_num = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "clk");
    if (JS_IsNumber(val))
        bus_config.sclk_io_num = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "host");
    if (JS_IsNumber(val))
        host = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);
#endif

	if (bus_config.sclk_io_num == -1) {
		JS_RETURN_NOTHING(ctx);
	}

	if (bus_config.mosi_io_num == -1 && bus_config.miso_io_num == -1) {
		JS_RETURN_NOTHING(ctx);
	}

	esp_err_t errRc = spi_bus_initialize(host, &bus_config, dma_chan);
	if (errRc != ESP_OK) {
		JS_RETURN_NOTHING(ctx);
	}
	JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_spi_bus_remove_device) {
    JS_ParameterPointer(ctx, 0);
	spi_device_handle_t *handle = (spi_device_handle_t *)JS_GetPointerParameter(ctx, 0);

	esp_err_t errRc = spi_bus_remove_device(*handle);
	free(handle);
	if (errRc != ESP_OK) {
		JS_RETURN_NOTHING(ctx);
	}
	JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_spi_device_transmit) {
    JS_ParameterPointer(ctx, 0);
	spi_device_handle_t *handle = (spi_device_handle_t *)JS_GetPointerParameter(ctx, 0);

    js_size_t size = 0;
    void *data = JS_GetBufferParameter(ctx, 1, &size);
    if (JS_ParameterCount(ctx) > 2) {
        JS_ParameterNumber(ctx, 2);

        js_size_t len = JS_GetIntParameter(ctx, 2);
        if ((len > 0) && (len < size))
            size = len;
    }

	spi_transaction_t trans_desc;
	trans_desc.flags     = 0;
	trans_desc.cmd       = 0;
	trans_desc.addr      = 0;
	trans_desc.length    = size * 8;
	trans_desc.rxlength  = 0;
	trans_desc.user      = NULL;
	trans_desc.tx_buffer = data;
	trans_desc.rx_buffer = data;

	esp_err_t errRc = spi_device_transmit(*handle, &trans_desc);
	if (errRc != ESP_OK) {
		JS_RETURN_NOTHING(ctx);
	}

	JS_RETURN_NOTHING(ctx);
}


void register_spi_functions(void *main_loop, void *js_ctx) {
    register_object(js_ctx, "spi",
        "addDevice", js_spi_bus_add_device,
	    "busFree", js_spi_bus_free,
	    "busInitialize", js_spi_bus_initialize,
	    "removeDevice", js_spi_bus_remove_device,
	    "transmit", js_spi_device_transmit,
        NULL, NULL
    );
    JS_EvalSimple(js_ctx, "spi.constants={SPI_HOST:0,HSPI_HOST:1,VSPI_HOST:2};");
}
