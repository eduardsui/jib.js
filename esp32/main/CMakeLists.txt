set(COMPONENT_SRCS "../../src/jib.c"
                   "../../src/builtins.c"
                   "../../src/misc/http_parser.c"
                   "../../src/builtins_io.c"
                   "../../src/builtins_socket.c"
                   "../../src/builtins_crypto.c"
                   "../../src/builtins_gpio.c"
                   "../../src/builtins_nvs.c"
                   "../../src/builtins_wifi.c"
                   "../../src/builtins_spi.c"
                   "../../src/builtins_i2c.c"
                   "../../src/builtins_ledc.c"
                   "../../src/builtins_adc.c"
                   "../../src/builtins_serial.c"
                   "../../src/builtins_esp32.c"
                   "../../src/misc/esp32/lora.c"
                   "../../src/builtins_tft.c"
                   "../../src/misc/esp32/tft/tft.c"
                   "../../src/misc/esp32/tft/tftspi.c"
                   "../../src/misc/esp32/tft/spi_master_lobo.c"
                   "../../src/misc/esp32/tft/DejaVuSans18.c"
                   "../../src/misc/esp32/tft/DejaVuSans24.c"
                   "../../src/misc/esp32/tft/Ubuntu16.c"
                   "../../src/misc/esp32/tft/comic24.c"
                   "../../src/misc/esp32/tft/minya24.c"
                   "../../src/misc/esp32/tft/SmallFont.c"
                   "../../src/misc/esp32/tft/def_small.c"
                   "../../src/misc/esp32/tft/DefaultFont.c"
                   "../../src/misc/esp32/tft/tooney32.c"
                   "../../src/duktape/duktape.c")
set(COMPONENT_ADD_INCLUDEDIRS "../../src ")

register_component()
