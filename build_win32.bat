@echo Building ...
@gcc.exe -static-libgcc -O2 -DTLS_AMALGAMATION -DLTC_NO_ASM src/jib.c src/builtins.c src/builtins_io.c src/builtins_socket.c src/misc/tlse.c src/misc/http_parser.c src/duktape/duktape.c src/ui/win32/htmlwindow.c -lws2_32  -liphlpapi -lwinmm -lole32 -loleaut32 -luuid -lshlwapi -lshlwapi -lwinscard -lcredui -o jib.exe
@echo done