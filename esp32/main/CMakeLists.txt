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
                   "../../src/builtins_esp32.c"
                   "../../src/misc/esp32/lora.c"
                   "../../src/quickjs/cutils.c"
                   "../../src/quickjs/libbf.c"
                   "../../src/quickjs/libregexp.c"
                   "../../src/quickjs/libunicode.c"
                   "../../src/quickjs/quickjs.c")
set(COMPONENT_ADD_INCLUDEDIRS "../../src ")

register_component()
