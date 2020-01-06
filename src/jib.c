#include "builtins.h"

#ifdef WITH_DUKTAPE
    void on_error(void *udata, const char *msg) {
        char func_name[50];
        log_log(5, func_name, native_line_number(NULL, -1, func_name, sizeof(func_name)), duk_safe_to_stacktrace(js(), -2));
        exit(0);
    }
#endif

int main(int argc, char *argv[], char *envp[]) {
    char buf[1024];

    struct doops_loop *main_loop = loop_new();
    JS_CONTEXT ctx = JS_CreateContext(on_error);
    register_builtins(main_loop, ctx, argc, argv, envp);

    if (argc > 1) {
        JS_Eval_File(ctx, argv[1]);
    } else {
        fprintf(stdout, "jib.js v0.1\n\n");

        while (fgets(buf, sizeof(buf), stdin) != NULL) {
            if (buf[0])
                JS_Eval(ctx, buf, "[cli]");
        }
    }

    loop_run(main_loop);

    JS_DestroyContext(ctx);

    loop_free(main_loop);

    return 0;
}

#ifdef ESP32
void app_main() {
    char *argv[] = { "rainbow", "::esp32" };
    main(2, argv, NULL);
}
#endif
