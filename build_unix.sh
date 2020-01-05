#!/bin/bash
echo "Building ..."
gcc -O2 -DNO_UI -DTLS_AMALGAMATION src/jib.c src/builtins.c src/builtins_io.c src/builtins_socket.c src/misc/tlse.c src/misc/http_parser.c src/duktape/duktape.c -lm -o jib
echo "done"
