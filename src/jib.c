#include "builtins.h"

#ifdef WITH_DUKTAPE
    void on_error(void *udata, const char *msg) {
        char func_name[50];
        log_log(5, func_name, native_line_number(NULL, -1, func_name, sizeof(func_name)), duk_safe_to_stacktrace(js(), -2));
        exit(-1);
    }
#else
    void on_error(const char *filename, const char *msg) {
        log_log(5, filename, 0, msg);
        exit(-1);
    }
#endif

int main(int argc, char *argv[], char *envp[]) {
    struct doops_loop *main_loop = loop_new();
    loop_io_wait(main_loop, 0);

    JS_CONTEXT ctx = JS_CreateContext(on_error);
    register_builtins(main_loop, ctx, argc, argv, envp);

    if (argc > 1) {
        JS_Eval_File(ctx, argv[1]);
    } else {
        JS_Eval_File(ctx, "::code");
    }

    loop_run(main_loop);

    JS_DestroyContext(ctx);

    loop_free(main_loop);

    js_deinit();

    return 0;
}

#ifdef ESP32
void app_main() {
    char *argv[] = { "jib", "::code" };
    main(2, argv, NULL);
}
#endif
