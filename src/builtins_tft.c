#include "builtins.h"
#include "builtins_adc.h"
#include "doops.h"

#include "misc/esp32/tft/tftspi.h"
#include "misc/esp32/tft/tft.h"

#define SPI_BUS TFT_HSPI_HOST

spi_lobo_device_handle_t disp_spi;

JS_C_FUNCTION(js_tft_init) {
    spi_lobo_device_handle_t spi;

    TFT_PinsInit();

    spi_lobo_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 6*1024,
    };

    spi_lobo_device_interface_config_t devcfg = {
        .clock_speed_hz = 8000000,
        .mode = 0,
        .spics_io_num = -1,
        .spics_ext_io_num = PIN_NUM_CS,
        .flags=LB_SPI_DEVICE_HALFDUPLEX,
    };

    spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);

    spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);

    disp_spi = spi;

    TFT_display_init();

    TFT_setFont(DEFAULT_FONT, NULL);
    TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
    TFT_resetclipwin();

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_rotate) {
    JS_ParameterBoolean(ctx, 0);

    if (JS_GetBooleanParameter(ctx, 0))
        TFT_setRotation(PORTRAIT);
    else
        TFT_setRotation(LANDSCAPE);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_print) {
    JS_ParameterString(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    const char *str = JS_GetStringParameter(ctx, 0);
    TFT_print((char *)str, JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2));
    JS_FreeString(ctx, str);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_set_font) {
    JS_ParameterString(ctx, 0);

    const char *str = JS_GetStringParameter(ctx, 0);
    int font = DEFAULT_FONT;
    if (!strcmp(str, "dejavu18"))
        font = DEJAVU18_FONT;
    else
    if (!strcmp(str, "dejavu24"))
        font = DEJAVU24_FONT;
    else
    if (!strcmp(str, "ubuntu16"))
        font = UBUNTU16_FONT;
    else
    if (!strcmp(str, "comic24"))
        font = COMIC24_FONT;
    else
    if (!strcmp(str, "minya24"))
        font = MINYA24_FONT;
    else
    if (!strcmp(str, "tooney32"))
        font = TOONEY32_FONT;
    else
    if (!strcmp(str, "small"))
        font = SMALL_FONT;
    else
    if (!strcmp(str, "7seg"))
        font = FONT_7SEG;
    JS_FreeString(ctx, str);

    TFT_setFont(font, NULL);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_fill_window) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 0);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 1);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 2);

    TFT_fillWindow(color);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_fill_screen) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 0);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 1);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 2);

    TFT_fillScreen(color);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_pixel) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    JS_ParameterNumber(ctx, 3);
    JS_ParameterNumber(ctx, 4);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 2);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 3);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 4);

    TFT_drawPixel(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), color, 1);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_rectangle) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    JS_ParameterNumber(ctx, 3);
    JS_ParameterNumber(ctx, 4);
    JS_ParameterNumber(ctx, 5);
    JS_ParameterNumber(ctx, 6);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 4);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 5);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 6);

    TFT_drawRect(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), JS_GetIntParameter(ctx, 3), color);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_fill_rectangle) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    JS_ParameterNumber(ctx, 3);
    JS_ParameterNumber(ctx, 4);
    JS_ParameterNumber(ctx, 5);
    JS_ParameterNumber(ctx, 6);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 4);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 5);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 6);

    TFT_fillRect(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), JS_GetIntParameter(ctx, 3), color);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_line) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    JS_ParameterNumber(ctx, 3);
    JS_ParameterNumber(ctx, 4);
    JS_ParameterNumber(ctx, 5);
    JS_ParameterNumber(ctx, 6);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 4);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 5);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 6);

    TFT_drawLine(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), JS_GetIntParameter(ctx, 3), color);

    JS_RETURN_NOTHING(ctx);
}


JS_C_FUNCTION(js_tft_circle) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    JS_ParameterNumber(ctx, 3);
    JS_ParameterNumber(ctx, 4);
    JS_ParameterNumber(ctx, 5);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 3);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 4);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 5);

    TFT_drawCircle(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), color);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_fill_circle) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    JS_ParameterNumber(ctx, 3);
    JS_ParameterNumber(ctx, 4);
    JS_ParameterNumber(ctx, 5);

    color_t color;
    color.r = (uint8_t)JS_GetIntParameter(ctx, 3);
    color.g = (uint8_t)JS_GetIntParameter(ctx, 4);
    color.b = (uint8_t)JS_GetIntParameter(ctx, 5);

    TFT_fillCircle(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), color);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_set_clip_win) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    JS_ParameterNumber(ctx, 3);

    TFT_setclipwin(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2), JS_GetIntParameter(ctx, 3));

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_save) {
    TFT_saveClipWin();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_reset) {
    TFT_resetclipwin();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_restore) {
    TFT_restoreClipWin();
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_set_color) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    _fg.r = (uint8_t)JS_GetIntParameter(ctx, 0);
    _fg.g = (uint8_t)JS_GetIntParameter(ctx, 1);
    _fg.b = (uint8_t)JS_GetIntParameter(ctx, 2);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_set_bg_color) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    _bg.r = (uint8_t)JS_GetIntParameter(ctx, 0);
    _bg.g = (uint8_t)JS_GetIntParameter(ctx, 1);
    _bg.b = (uint8_t)JS_GetIntParameter(ctx, 2);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_invert_colors) {
    JS_ParameterNumber(ctx, 0);
    TFT_invertDisplay(JS_GetIntParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_jpeg) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);

    void *buf;
    js_size_t sz;

    buf = JS_GetBufferParameter(ctx, 2, &sz);
    if (buf)
        TFT_jpg_image(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), 0, NULL, (uint8_t *)buf, sz);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_jpeg_file) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterString(ctx, 2);

    const char *fname = JS_GetStringParameter(ctx, 2);
    TFT_jpg_image(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), 0, (char *)fname, NULL, 0);
    JS_FreeString(ctx, fname);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_bmp) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);

    void *buf;
    js_size_t sz;

    buf = JS_GetBufferParameter(ctx, 2, &sz);
    if (buf)
        TFT_bmp_image(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), 0, NULL, (uint8_t *)buf, sz);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_bmp_file) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterString(ctx, 2);

    const char *fname = JS_GetStringParameter(ctx, 2);
    TFT_bmp_image(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), 0, (char *)fname, NULL, 0);
    JS_FreeString(ctx, fname);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_width) {
    JS_RETURN_NUMBER(ctx, _width);
}

JS_C_FUNCTION(js_tft_height) {
    JS_RETURN_NUMBER(ctx, _height);
}

JS_C_FUNCTION(js_tft_font_height) {
    int height = TFT_getfontheight();
    JS_RETURN_NUMBER(ctx, height);
}

JS_C_FUNCTION(js_tft_text_width) {
    const char *str = JS_GetStringParameter(ctx, 0);
    int width = TFT_getStringWidth((char *)str);
    JS_FreeString(ctx, str);
    JS_RETURN_NUMBER(ctx, width);
}

JS_C_FUNCTION(js_tft_deinit) {
    spi_lobo_device_deselect(disp_spi);
    int err = spi_lobo_bus_remove_device(disp_spi);
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_tft_text_wrap) {
    JS_ParameterBoolean(ctx, 0);
    text_wrap = JS_GetBooleanParameter(ctx, 0);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tft_grayscale) {
    JS_ParameterBoolean(ctx, 0);
    gray_scale = JS_GetBooleanParameter(ctx, 0);
    JS_RETURN_NOTHING(ctx);
}

void register_tft_functions(void *main_loop, void *js_ctx) {
    register_object(js_ctx, "tft",
        "init", js_tft_init,
        "rotate", js_tft_rotate,
        "print", js_tft_print,
        "setFont", js_tft_set_font,
        "line", js_tft_line,
        "circle", js_tft_circle,
        "fillCircle", js_tft_fill_circle,
        "rect", js_tft_rectangle,
        "fillRect", js_tft_fill_rectangle,
        "fillWindow", js_tft_fill_window,
        "fillScreen", js_tft_fill_screen,
        "pixel", js_tft_pixel,
        "setClipWindow", js_tft_set_clip_win,
        "save", js_tft_save,
        "reset", js_tft_reset,
        "restore", js_tft_restore,
        "setColor", js_tft_set_color,
        "setBgColor", js_tft_set_bg_color,
        "invertColors", js_tft_invert_colors,
        "jpeg", js_tft_jpeg,
        "jpegFile", js_tft_jpeg_file,
        "bmp", js_tft_bmp,
        "bmpFile", js_tft_bmp_file,
        "width", js_tft_width,
        "height", js_tft_height,
        "fontHeight", js_tft_font_height,
        "textWidth", js_tft_text_width,
        "setTextWrap", js_tft_text_wrap,
        "grayscale", js_tft_grayscale,
        "deinit", js_tft_deinit,
        NULL, NULL
    );
}
