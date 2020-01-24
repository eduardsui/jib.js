#include "builtins.h"
#include "builtins_serial.h"
#include "doops.h"

#include <driver/uart.h>

JS_C_FUNCTION(js_serial_configure) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterObject(ctx, 1);

	int port;
	int baud;
	int rx_buffer_size;
	int tx_buffer_size;
	int tx_pin = UART_PIN_NO_CHANGE;
	int rx_pin = UART_PIN_NO_CHANGE;
	esp_err_t errRc;

	port = JS_GetIntParameter(ctx, 0);
	if (port < 0 || port > 2) {
		JS_RETURN_NUMBER(ctx, -1);
	}
#ifdef WITH_DUKTAPE
	if (duk_get_prop_string(ctx, 1, "baud") != 1) {
		duk_pop(ctx);
		baud = 115200;
	} else {
		baud = duk_get_int(ctx, -1);
		duk_pop(ctx);
	}

	if (duk_get_prop_string(ctx, 1, "rxBufferSize") != 1) {
		duk_pop(ctx);
		rx_buffer_size = UART_FIFO_LEN + 1;
	} else {
		rx_buffer_size = duk_get_int(ctx, -1);
		duk_pop(ctx);
		if (rx_buffer_size <= UART_FIFO_LEN) {
			rx_buffer_size = UART_FIFO_LEN + 1;
		}
	}

	if (duk_get_prop_string(ctx, 1, "txBufferSize") != 1) {
		duk_pop(ctx);
		tx_buffer_size = 0;
	} else {
		tx_buffer_size = duk_get_int(ctx, -1);
		duk_pop(ctx);
	}

	if (duk_get_prop_string(ctx, 1, "rxPin") == 1) {
		rx_pin = duk_get_int(ctx, -1);
		duk_pop(ctx);
	} else {
		duk_pop(ctx);
	}

	if (duk_get_prop_string(ctx, 1, "txPin") == 1) {
		tx_pin = duk_get_int(ctx, -1);
		duk_pop(ctx);
	} else {
		duk_pop(ctx);
	}
#else
    js_object_type obj = JS_GetObjectParameter(ctx, 1);
    js_object_type val = JS_GetPropertyStr(ctx, obj, "baud");
    if (JS_IsNumber(val)) {
        baud = _JS_GetIntParameter(ctx, val);
    } else {
        baud = 115200;
    }
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "rxBufferSize");
    if (JS_IsNumber(val)) {
        rx_buffer_size = _JS_GetIntParameter(ctx, val);
		if (rx_buffer_size <= UART_FIFO_LEN)
			rx_buffer_size = UART_FIFO_LEN + 1;
    } else {
        rx_buffer_size = UART_FIFO_LEN + 1;
    }
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "txBufferSize");
    if (JS_IsNumber(val)) {
        tx_buffer_size = _JS_GetIntParameter(ctx, val);
    } else {
        tx_buffer_size = 0;
    }
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "rxPin");
    if (JS_IsNumber(val))
        rx_pin = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);

    val = JS_GetPropertyStr(ctx, obj, "txPin");
    if (JS_IsNumber(val))
        tx_pin = _JS_GetIntParameter(ctx, val);
    JS_FreeValue(ctx, val);
#endif

	uart_config_t myUartConfig;
	myUartConfig.baud_rate = baud;
	myUartConfig.data_bits = UART_DATA_8_BITS;
	myUartConfig.parity = UART_PARITY_DISABLE;
	myUartConfig.stop_bits = UART_STOP_BITS_1;
	myUartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	myUartConfig.rx_flow_ctrl_thresh = 120;

	errRc = uart_param_config(port, &myUartConfig);
	if (errRc != ESP_OK) {
		JS_RETURN_NUMBER(ctx, errRc);
	}

	errRc = uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (errRc != ESP_OK) {
		JS_RETURN_NUMBER(ctx, errRc);
	}

	errRc = uart_driver_install(
		port, // Port
		rx_buffer_size, // RX buffer size
		tx_buffer_size, // TX buffer size
		10, // queue size
		NULL, // Queue
		0 // Interrupt allocation flags
	);
	JS_RETURN_NUMBER(ctx, errRc);
}

JS_C_FUNCTION(js_serial_read) {
    JS_ParameterNumber(ctx, 0);
	js_size_t size;
	esp_err_t errRc;
	int port = JS_GetIntParameter(ctx, 0);
	if (port < 0) {
		JS_RETURN_NUMBER(ctx, -1);
	}
	void *data = JS_GetBufferParameter(ctx, 1, &size);
	if (data == NULL) {
		JS_RETURN_NUMBER(ctx, -1);
	}

	size_t available;
	errRc = uart_get_buffered_data_len(port, &available);
	if (errRc != ESP_OK) {
		JS_RETURN_NUMBER(ctx, errRc);
	}
	if (available == 0) {
		JS_RETURN_NUMBER(ctx, 0);
	}
	if (available > size) {
		available = size;
	}

	int numRead = uart_read_bytes(port, data, available, 0);
	JS_RETURN_NUMBER(ctx, numRead);
}

JS_C_FUNCTION(js_serial_write) {
    JS_ParameterNumber(ctx, 0);
	js_size_t size;
	int port = JS_GetIntParameter(ctx, 0);
	if (port < 0) {
		JS_RETURN_NUMBER(ctx, -1);
	}
	const void *data = JS_GetBufferParameter(ctx, 1, &size);
	if ((data == NULL) || (size == 0)) {
		JS_RETURN_NUMBER(ctx, -1);
	}
	esp_err_t errRc = uart_write_bytes(port, data, size);
	JS_RETURN_NUMBER(ctx, errRc);
}

void register_uart_functions(void *main_loop, void *js_ctx) {
    register_object(js_ctx, "uart",
        "configure", js_serial_configure,
        "read", js_serial_read,
        "write", js_serial_write,
        NULL, NULL
    );
}
