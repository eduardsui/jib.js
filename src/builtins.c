#include "builtins.h"
#include "builtins_js.h"
#include "modules.h"

#include <unistd.h>
#ifndef _WIN32
    #include <sys/types.h>
    #ifndef ESP32
        #include <sys/sysctl.h>
        #include <signal.h>
    #endif
#endif

#ifndef NO_SOCKET
#include "builtins_socket.h"
#endif

#ifndef NO_IO
#include "builtins_io.h"
#endif

#ifdef WITH_GPIO
#include "builtins_gpio.h"
#endif

#ifdef WITH_NVS
#include "builtins_nvs.h"
#endif

#ifdef WITH_WIFI
#include "builtins_wifi.h"
#endif

#ifndef NO_HTTP
    #include "misc/http_parser.h"
#endif

#ifdef ESP32
    #include "builtins_esp32.h"
#endif

#ifndef NO_UI
#include "ui/htmlwindow.h"

static int ui_initialized;
#endif

#ifndef NO_COMPRESSION
#include "miniz.c"
#endif

#ifdef _WIN32
    #include <wincrypt.h>
#endif

#define DUK_STRING_PUSH_SAFE (1 << 0)

static const char *level_names[] = { "TRACE", "DEBUG", "LOG", "WARN", "ERROR", "FATAL" };
static const char *level_colors[] = { "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m" };
#ifdef USE_SYSLOG
static int syslog_levels[] = { LOG_DEBUG, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR, LOG_CRIT };
#endif

static JS_CONTEXT js_ctx;
static struct doops_loop *main_loop;

static struct {
#ifdef USE_SYSLOG
    int        logfile;
#else
    FILE       *logfile;
#endif
    int        loglevel;
    char       colors;
    char       quiet;
    int        links;
    char       initialized;
} log_context;


#ifndef WITH_DUKTAPE
js_object_type global_stash(JS_CONTEXT ctx) {
    void *opaque = JS_GetContextOpaque(ctx);
    if (!opaque) {
        js_object_type *obj = (js_object_type *)malloc(sizeof(js_object_type));
        *obj = JS_NewPlainObject(ctx);
        JS_SetContextOpaque(ctx, obj);
        opaque = obj;
    }

    return *(js_object_type *)opaque;
}
#endif

static void log_init() {
#ifdef _WIN32
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD fdwSaveOldMode;
    GetConsoleMode(hStdout, &fdwSaveOldMode);
    if (SetConsoleMode(hStdout, fdwSaveOldMode | 0x0004))
        log_context.colors = 1;
    else
        log_context.colors = 0;
#else
    log_context.colors = 1;
#endif
    log_context.initialized = 1;
}

void log_log(int level, const char *file, int line, const char *data) {
    if (!log_context.initialized)
        log_init();
    if ((level < log_context.loglevel) || (level < 0) || (level > 5))
        return;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t now = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

    time_t t = (time_t)(now/1000);
    struct tm *lt = localtime(&t);

    if (!log_context.quiet) {
        char buf[16];
        buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
        if (log_context.colors)
            fprintf(stderr, "%s.%03d %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m %s\n", buf, (int)(now % 1000), level_colors[level], level_names[level], file, line, data);
        else
            fprintf(stderr, "%s.%03d %-5s %s:%d: %s\n", buf, (int)(now % 1000), level_names[level], file, line, data);
        fflush(stderr);
    }

    if (log_context.logfile) {
#ifdef USE_SYSLOG
        syslog(syslog_levels[level], "%s:%d: %s\n", file, line, data);
#else
        char buf[32];
        buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
        fprintf(log_context.logfile, "%s.%03d %-5s %s:%d: %s\n", buf, (int)(now % 1000), level_names[level], file, line, data);
        fflush(log_context.logfile);
#endif
    }
}

#ifdef WITH_DUKTAPE
int native_line_number(JS_CONTEXT ctx, int level, char *func_name, int max_len) {
    if (!ctx)
        ctx = js_ctx;
    if (!ctx) {
        if ((func_name) && (max_len))
            func_name[0] = 0;
        return 0;
    }
    duk_inspect_callstack_entry(ctx, level);
    if (duk_is_undefined(ctx, -1)) {
        if ((func_name) && (max_len > 0))
            func_name[0] = 0;
        return 1;
    }
    duk_get_prop_string(ctx, -1, "lineNumber");
    int line_number = (int)duk_to_int(ctx, -1);
    if ((func_name) && (max_len > 0)) {
        func_name[0] = 0;
        duk_pop(ctx);
        duk_get_prop_string(ctx, -1, "function");
        if (duk_is_function(ctx, -1)) {
            duk_get_prop_string(ctx, -1, "fileName");
            const char *filename = duk_to_string(ctx, -1);
            duk_pop(ctx);
            duk_get_prop_string(ctx, -1, "name");
            const char *name = duk_to_string(ctx, -1);
            const char *anon_name = "[anon]";
            if ((!name) || (!name[0]))
                name = anon_name;

            if (!strcmp(name, "eval"))
                strncpy(func_name, filename, max_len);
            else
                snprintf(func_name, max_len, "%s() %s", name, filename);
            duk_pop(ctx);
        } else
            strncpy(func_name, duk_to_string(ctx, -1), max_len);
    }
    duk_pop_2(ctx);
    return line_number;
}
#endif

static int setInterval_callback(struct doops_loop *loop) {
    char func_buffer[50]; 
    unsigned int callbackIndex = (unsigned int)(uintptr_t)loop_event_data(loop);
    snprintf(func_buffer, sizeof(func_buffer), "interval%u", callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_get_prop_string(js_ctx, 1, func_buffer);

    duk_call(js_ctx, 0);

    if (duk_to_boolean(js_ctx, -1)) {
        duk_push_global_stash(js_ctx);
        duk_push_string(js_ctx, func_buffer);
        duk_del_prop(js_ctx, -3);
        duk_pop_3(js_ctx);
        return 1;
    }
    duk_pop_2(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer);
    js_object_type ret_value = JS_Call(js_ctx, obj, obj, 0, NULL);
    if ((JS_IsBool(ret_value)) && (_JS_GetBooleanParameter(js_ctx, ret_value))) {
        JS_DeleteProperty(js_ctx, global_stash(js_ctx), JS_ValueToAtom(js_ctx, obj), 0);
        JS_FreeValue(js_ctx, ret_value);
        return 1;
    }
    JS_FreeValue(js_ctx, ret_value);
#endif
    return 0;
}

static int setTimeout_callback(struct doops_loop *loop) {
    char func_buffer[50];
    unsigned int callbackIndex = (unsigned int)(uintptr_t)loop_event_data(loop);
    snprintf(func_buffer, sizeof(func_buffer), "timeout%u", callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_get_prop_string(js_ctx, 1, func_buffer);

    duk_call(js_ctx, 0);

    duk_push_global_stash(js_ctx);
    duk_push_string(js_ctx, func_buffer);
    duk_del_prop(js_ctx, -2);
    duk_pop_3(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer);
    JS_FreeValue(js_ctx, JS_Call(js_ctx, obj, obj, 0, NULL));
    JS_DeleteProperty(js_ctx, global_stash(js_ctx), JS_ValueToAtom(js_ctx, obj), 0);
#endif
    return 1;
}

static int setImmediate_callback(struct doops_loop *loop) {
    char func_buffer[50]; 
    unsigned int callbackIndex = (unsigned int)(uintptr_t)loop_event_data(loop);
    snprintf(func_buffer, sizeof(func_buffer), "immediate%u", callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_get_prop_string(js_ctx, 1, func_buffer);

    duk_call(js_ctx, 0);

    duk_push_global_stash(js_ctx);
    duk_push_string(js_ctx, func_buffer);
    duk_del_prop(js_ctx, -2);
    duk_pop_3(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer);
    JS_FreeValue(js_ctx, JS_Call(js_ctx, obj, obj, 0, NULL));
    JS_DeleteProperty(js_ctx, global_stash(js_ctx), JS_ValueToAtom(js_ctx, obj), 0);
#endif
    return 1;
}

JS_C_FUNCTION_FORWARD(registerCallback, const char *prefix, doop_callback callback) {
    char func_buffer[50];
    // callbackIndex must be 1-based (to correctly work with loop_remove)
    JS_ParameterFunction(ctx, 0);
    JS_ParameterNumber(ctx, 1);

    static unsigned int callbackIndex;
    callbackIndex ++;
    // wrap around?
    if (!callbackIndex)
        callbackIndex ++;

    double ms = JS_GetNumberParameter(ctx, 1);
    snprintf(func_buffer, sizeof(func_buffer), "%s%u", prefix, callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(ctx);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, func_buffer);
    duk_pop_2(js_ctx);
#else
    JS_SetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer, argv[0]);
#endif
    loop_add(main_loop, callback, (int)ms, (void *)(uintptr_t)callbackIndex);

    JS_RETURN_NUMBER(ctx, callbackIndex);
}

JS_C_FUNCTION(setInterval) {
    return registerCallback(JS_C_FORWARD_PARAMETERS, "interval", setInterval_callback);
}

JS_C_FUNCTION(clearInterval) {
    JS_ParameterNumber(ctx, 0);
    uintptr_t event_id = (uintptr_t)JS_GetNumberParameter(ctx, 0);
    loop_remove(main_loop, setInterval_callback, (void *)event_id);

    char func_buffer[50];
    unsigned int callbackIndex = (unsigned int)event_id;
    snprintf(func_buffer, sizeof(func_buffer), "interval%u", callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_del_prop_string(js_ctx, -1, func_buffer);
    duk_pop(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer);
    JS_DeleteProperty(js_ctx, global_stash(js_ctx), JS_ValueToAtom(js_ctx, obj), 0);
#endif

	JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(setTimeout) {
    return registerCallback(JS_C_FORWARD_PARAMETERS, "timeout", setTimeout_callback);
}

JS_C_FUNCTION(clearTimeout) {
    JS_ParameterNumber(ctx, 0);
    uintptr_t event_id = (uintptr_t)JS_GetNumberParameter(ctx, 0);
    loop_remove(main_loop, setTimeout_callback, (void *)event_id);

    char func_buffer[50];
    unsigned int callbackIndex = (unsigned int)event_id;
    snprintf(func_buffer, sizeof(func_buffer), "timeout%u", callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_del_prop_string(js_ctx, -1, func_buffer);
    duk_pop(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer);
    JS_DeleteProperty(js_ctx, global_stash(js_ctx), JS_ValueToAtom(js_ctx, obj), 0);
#endif

	JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(setImmediate) {
    char func_buffer[50]; 
    // callbackIndex must be 1-based (to correctly work with loop_remove)
    static unsigned int callbackIndex;
    callbackIndex ++;

    // wrap around?
    if (!callbackIndex)
        callbackIndex ++;

    JS_ParameterFunction(ctx, 0);
    snprintf(func_buffer, sizeof(func_buffer), "immediate%u", callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(ctx);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, func_buffer);
    duk_pop_2(js_ctx);
#else
    JS_SetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer, argv[0]);
#endif
    loop_add(main_loop, setImmediate_callback, (int)0, (void *)(uintptr_t)callbackIndex);

    JS_RETURN_NUMBER(ctx, callbackIndex);
}

JS_C_FUNCTION(randomBytes) {
    JS_ParameterNumber(ctx, 0);
    int len = JS_GetIntParameter(ctx, 0);
    if (len <= 0)
        JS_RETURN_UNDEFINED(ctx);

    unsigned char *key = (unsigned char *)malloc(len);
    if (!key)
        JS_RETURN_UNDEFINED(ctx);

#ifdef __APPLE__
    for (int i = 0; i < len; i++) {
        unsigned int v = arc4random() % 0x100;
        key[i] = (char)v;
    }
    JS_RETURN_BUFFER_FREE_VAL(ctx, key, len);
#else
#ifdef _WIN32
    HCRYPTPROV hProvider = 0;
    if (CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        if (CryptGenRandom(hProvider, len, (BYTE *)key)) {
            CryptReleaseContext(hProvider, 0);
            JS_RETURN_BUFFER_FREE_VAL(ctx, key, len);
        }
        CryptReleaseContext(hProvider, 0);
        free(key);
    }
#else
#ifdef ESP32
    esp_fill_random(key, len);
#else
    FILE *fp = fopen("/dev/urandom", "r");
    if (fp) {
        int key_len = fread(key, 1, len, fp);
        fclose(fp);
        if (key_len == len)
            JS_RETURN_BUFFER_FREE_VAL(ctx, key, len);
    }
#endif
#endif
#endif
    JS_RETURN_UNDEFINED(ctx);
}

JS_C_FUNCTION(clearImmediate) {
    JS_ParameterNumber(ctx, 0);
    uintptr_t event_id = (uintptr_t)JS_GetNumberParameter(ctx, 0);
    loop_remove(main_loop, setImmediate_callback, (void *)event_id);

    char func_buffer[50];
    unsigned int callbackIndex = (unsigned int)event_id;
    snprintf(func_buffer, sizeof(func_buffer), "immediate%u", callbackIndex); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_del_prop_string(js_ctx, -1, func_buffer);
    duk_pop(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), func_buffer);
    JS_DeleteProperty(js_ctx, global_stash(js_ctx), JS_ValueToAtom(js_ctx, obj), 0);
#endif

	JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION_FORWARD(native_print, int level) {
#ifdef WITH_DUKTAPE
    char func_name[50];
    int line_number = native_line_number(ctx, -2, func_name, sizeof(func_name));
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
    log_log(level, func_name, line_number, duk_safe_to_string(ctx, -1));
#else
    const char *text = "";
    if (JS_ParameterCount(ctx) > 0)
        text = JS_GetAsStringParameter(ctx, 0);

    log_log(level, "(none)", 0, text);
#endif

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_log) {
	return native_print(JS_C_FORWARD_PARAMETERS, 2);
}

JS_C_FUNCTION(native_warning) {
    return native_print(JS_C_FORWARD_PARAMETERS, 3);
}

JS_C_FUNCTION(native_error) {
    return native_print(JS_C_FORWARD_PARAMETERS, 4);
}

void register_global_function(JS_CONTEXT ctx, const char *func_name, js_c_function function, int nargs) {
    if (!ctx)
        ctx = js_ctx;
    if (!ctx)
        return;
#ifdef WITH_DUKTAPE
    duk_push_c_function(ctx, function, nargs);
	duk_put_global_string(ctx, func_name);
#else
    JS_ObjectSetFunction(ctx, JS_GetGlobalObject(ctx), func_name, function, JS_VARARGS);
#endif
}

void register_object(JS_CONTEXT ctx, const char *object_name, ...) {
    va_list ap;
#ifdef WITH_DUKTAPE
    duk_push_object(ctx);

    va_start(ap, object_name);
    while (1) {
        const char *func_name = va_arg(ap, const char *);
        if (!func_name)
            break;

        js_c_function function = va_arg(ap, js_c_function);
        if (!function)
            break;

        duk_push_c_function(ctx, function, JS_VARARGS);
        duk_put_prop_string(ctx, -2, func_name);        
    }
    va_end(ap);

    duk_put_global_string(ctx, object_name);
#else
    JSValue obj = JS_NewPlainObject(ctx);

    JS_ObjectSetObject(ctx, JS_GetGlobalObject(ctx), object_name, obj);
    va_start(ap, object_name);
    while (1) {
        const char *func_name = va_arg(ap, const char *);
        if (!func_name)
            break;

        js_c_function function = va_arg(ap, js_c_function);
        if (!function)
            break;

        JS_ObjectSetFunction(ctx, obj, func_name, function, JS_VARARGS);
    }
    va_end(ap);
#endif
}

JS_C_FUNCTION(native_quit) {
    helper_notify(ctx, "app", "onexit");
    if (main_loop) {
        loop_quit(main_loop);
    } else {
        exit(0);
    }
	JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_exit) {
    helper_notify(ctx, "app", "onexit");
    exit((int)JS_GetNumberParameter(ctx, -1));
	JS_RETURN_NOTHING(ctx);
}

#ifndef NO_HTTP
struct http_parser_container {
    js_object_type obj_id;
    js_object_type header_obj;
    const char *last_header;
    size_t last_header_len;
};

int on_header_field(struct http_parser *_, const char *at, size_t len) {
    ((struct http_parser_container *)(_->data))->last_header = at;
    ((struct http_parser_container *)(_->data))->last_header_len = len;
    return 0;
}

int on_header_value(struct http_parser *_, const char *at, size_t len) {
#ifdef WITH_DUKTAPE
    if (!((struct http_parser_container *)(_->data))->header_obj)
        ((struct http_parser_container *)(_->data))->header_obj = JS_NewPlainObject(js());
#else
    if (JS_IsUndefined(((struct http_parser_container *)(_->data))->header_obj))
        ((struct http_parser_container *)(_->data))->header_obj = JS_NewPlainObject(js());
#endif

    if (((struct http_parser_container *)(_->data))->last_header)
        JS_ObjectSetStringLenLen(js(), ((struct http_parser_container *)(_->data))->header_obj, ((struct http_parser_container *)(_->data))->last_header, ((struct http_parser_container *)(_->data))->last_header_len, at, len);

    return 0;
}

int on_url(http_parser *_, const char *at, size_t len) {
    JS_ObjectSetStringLen(js(), ((struct http_parser_container *)(_->data))->obj_id, "url", at, len);
    return 0;
}

int on_headers_complete(http_parser *_) {
#ifdef WITH_DUKTAPE
    if (((struct http_parser_container *)(_->data))->header_obj)
        JS_ObjectSetObject(js(), ((struct http_parser_container *)(_->data))->obj_id, "headers", ((struct http_parser_container *)(_->data))->header_obj);
#else
    if (!JS_IsUndefined(((struct http_parser_container *)(_->data))->header_obj))
        JS_ObjectSetObject(js(), ((struct http_parser_container *)(_->data))->obj_id, "headers", ((struct http_parser_container *)(_->data))->header_obj);
#endif
    return 0;
}

static http_parser_settings settings_null = {
    .on_message_begin = 0
    ,.on_header_field = on_header_field
    ,.on_header_value = on_header_value
    ,.on_url = on_url
    ,.on_status = 0
    ,.on_body = 0
    ,.on_headers_complete = on_headers_complete
    ,.on_message_complete = 0
    ,.on_chunk_header = 0
    ,.on_chunk_complete = 0
};

JS_C_FUNCTION_FORWARD(parseHeader, int type) {
    if (JS_ParameterCount(ctx) == 0)
        JS_RETURN_NOTHING(ctx);

    js_size_t header_len = 0;
    const char *header = JS_GetAsStringParameterLen(ctx, 0, &header_len);

    struct http_parser parser;
    http_parser_init(&parser, type);

    struct http_parser_container data;

    js_object_type obj_id = JS_NewPlainObject(ctx);
    data.obj_id = obj_id;
    data.last_header = NULL;
    data.last_header_len = 0;
#ifdef WITH_DUKTAPE
    data.header_obj = 0;
#else
    data.header_obj = JS_UNDEFINED;
#endif
    parser.data = &data;

    size_t nparsed = http_parser_execute(&parser, &settings_null, header, header_len);
    if (nparsed > 0) {
        JS_ObjectSetBoolean(ctx, obj_id, "upgrade", parser.upgrade);
        if (type == HTTP_REQUEST)
            JS_ObjectSetNumber(ctx, obj_id, "statusCode", parser.status_code);
        JS_ObjectSetNumber(ctx, obj_id, "errno", parser.http_errno);
        switch (parser.method) {
            case HTTP_GET:
                JS_ObjectSetString(ctx, obj_id, "method", "GET");
                break;
            case HTTP_HEAD:
                JS_ObjectSetString(ctx, obj_id, "method", "HEAD");
                break;
            case HTTP_POST:
                JS_ObjectSetString(ctx, obj_id, "method", "POST");
                break;
            case HTTP_PUT:
                JS_ObjectSetString(ctx, obj_id, "method", "PUT");
                break;
            case HTTP_DELETE:
                JS_ObjectSetString(ctx, obj_id, "method", "DELETE");
                break;
            default:
                JS_ObjectSetNumber(ctx, obj_id, "method", parser.method);
                break;
        }
        if ((int64_t)parser.content_length >= 0)
            JS_ObjectSetNumber(ctx, obj_id, "contentLength", parser.content_length);
        JS_ObjectSetNumber(ctx, obj_id, "httpVersion", (double)parser.http_major + ((double)parser.http_minor)/10);
    }
#ifdef WITH_DUKTAPE
    return 1;
#else
    return obj_id;
#endif
}

JS_C_FUNCTION(parseRequest) {
    return parseHeader(JS_C_FORWARD_PARAMETERS, HTTP_REQUEST);
}

JS_C_FUNCTION(parseResponse) {
    return parseHeader(JS_C_FORWARD_PARAMETERS, HTTP_RESPONSE);
}
#endif

#ifndef NO_UI
void gui_callback(void *window) {
    char wnd_buffer[50]; 
    snprintf(wnd_buffer, sizeof(wnd_buffer), "wnd%X", (unsigned int)(uintptr_t)window); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_get_prop_string(js_ctx, -1, wnd_buffer);

    if (duk_is_object(js_ctx, -1)) {
        duk_get_prop_string(js_ctx, -1, "onuievent");
        if (duk_is_callable(js_ctx, -1))
            duk_call(js_ctx, 0);
        duk_pop(js_ctx);
    }
    duk_pop(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), wnd_buffer);
    if (JS_IsObject(obj)) {
        js_object_type function_obj = JS_GetPropertyStr(js_ctx, obj, "onuievent");
        if (JS_IsFunction(js_ctx, function_obj))
            JS_FreeValue(js_ctx, JS_Call(js_ctx, function_obj, obj, 0, NULL));
    }
#endif
}

static int ui_callback(struct doops_loop *loop) {
    ui_app_iterate();
    if (!ui_window_count())
        return 1;
    return 0;
}

static void ui_close_callback(void *event_data, void *userdata) {
    char wnd_buffer[50]; 
    snprintf(wnd_buffer, sizeof(wnd_buffer), "wnd%X", (unsigned int)(uintptr_t)event_data); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_get_prop_string(js_ctx, -1, wnd_buffer);

    if (duk_is_object(js_ctx, -1)) {
        duk_push_pointer(js_ctx, NULL);
        duk_put_prop_string(js_ctx, -2, JS_HIDDEN_SYMBOL("__native_handle"));

        duk_get_prop_string(js_ctx, -1, "ondestroy");
        if (duk_is_callable(js_ctx, -1))
            duk_call(js_ctx, 0);

        duk_pop(js_ctx);
    }
    duk_push_global_stash(js_ctx);
    duk_push_string(js_ctx, wnd_buffer);
    duk_del_prop(js_ctx, -3);
    duk_pop_3(js_ctx);
#else
    js_object_type obj = JS_GetPropertyStr(js_ctx, global_stash(js_ctx), wnd_buffer);
    if (JS_IsObject(obj)) {
        JS_ObjectSetPointer(js_ctx, obj, JS_HIDDEN_SYMBOL("__native_handle"), NULL);
        js_object_type function_obj = JS_GetPropertyStr(js_ctx, obj, "ondestroy");
        if (JS_IsFunction(js_ctx, function_obj))
            JS_FreeValue(js_ctx, JS_Call(js_ctx, function_obj, obj, 0, NULL));
        JS_DeleteProperty(js_ctx, global_stash(js_ctx), JS_ValueToAtom(js_ctx, obj), 0);
    }
#endif
}

JS_C_FUNCTION(native_ui_window_close) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
#endif
    if (window_ptr)
        ui_window_close(window_ptr);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_maximize) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
#endif
    if (window_ptr)
        ui_window_maximize(window_ptr);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_minimize) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
#endif
    if (window_ptr)
        ui_window_minimize(window_ptr);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_restore) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
#endif
    if (window_ptr)
        ui_window_restore(window_ptr);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_top) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
#endif
    if (window_ptr)
        ui_window_top(window_ptr);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_content) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
    if (window_ptr) {
	    duk_push_string(ctx, " ");
	    duk_insert(ctx, 0);
	    duk_join(ctx, duk_get_top(ctx) - 1);
        ui_window_set_content(window_ptr, duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
    }
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
    if (window_ptr) {
        if (JS_ParameterCount(ctx) > 0)
            ui_window_set_content(window_ptr, JS_GetAsStringParameter(ctx, 0));
        else
            ui_window_set_content(window_ptr, "");
    }
#endif
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_js) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
    if (window_ptr) {
	    duk_push_string(ctx, " ");
	    duk_insert(ctx, 0);
	    duk_join(ctx, duk_get_top(ctx) - 1);
        ui_js(window_ptr, duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
    }
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
    if (window_ptr) {
        if (JS_ParameterCount(ctx) > 0)
            ui_js(window_ptr, JS_GetAsStringParameter(ctx, 0));
    }
#endif

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_execute) {
    int parameters = JS_ParameterCount(ctx);
    if (parameters < 1)
        JS_RETURN_NOTHING(ctx);

#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
#endif
    if (window_ptr) {
        const char *arguments[0x80];
        int i;
        if (parameters >= 0x80 - 1)
            parameters = 0x80 - 1;

        arguments[0] = 0;
        for (i = 1; i < parameters; i ++) {
            arguments[i - 1] = JS_GetAsStringParameter(ctx, i);
            arguments[i] = 0;
        }

        char *val = ui_call(window_ptr, JS_GetAsStringParameter(ctx, 0), arguments);
        if (val) {
#ifdef WITH_DUKTAPE
            duk_push_string(ctx, val);
            ui_free_string(val);
            return 1;
#else
            js_object_type jsval = JS_NewString(ctx, val);
            ui_free_string(val);
            return jsval;
#endif
        }
    }
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(native_ui_window_is_opened) {
#ifdef WITH_DUKTAPE
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, JS_HIDDEN_SYMBOL("__native_handle"));

    void *window_ptr = duk_get_pointer_default(ctx, -1, NULL);
    duk_pop_2(ctx);
#else
    void *window_ptr = _JS_GetPointerParameter(ctx, JS_GetPropertyStr(ctx, this_val, JS_HIDDEN_SYMBOL("__native_handle")));
#endif
    JS_RETURN_BOOLEAN(ctx, (window_ptr != 0));
}

JS_C_FUNCTION(native_alert) {
    char func_name[50];
#ifdef WITH_DUKTAPE
    int line_number = native_line_number(ctx, -2, func_name, sizeof(func_name));
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
    const char *text = duk_safe_to_string(ctx, -1);
    log_log(2, func_name, line_number, text);
#else
    const char *text = "";
    if (JS_ParameterCount(ctx) > 0)
        text = JS_GetAsStringParameter(ctx, 0);
#endif
    ui_message("", text, 1);
    JS_RETURN_NOTHING(ctx);
}
#endif

JS_C_FUNCTION(native_chdir) {
    JS_ParameterString(ctx, 0);
#ifndef NO_IO
    int err = chdir(JS_GetStringParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
#else
    JS_RETURN_NOTHING(ctx);
#endif
}

JS_C_FUNCTION(native_cwd) {
#ifndef NO_IO
    char cwd[PATH_MAX];
    if (getcwd(cwd, 0x100))
        JS_RETURN_STRING(ctx, cwd);
    JS_RETURN_UNDEFINED(ctx);
#else
    JS_RETURN_NOTHING(ctx);
#endif
}

JS_C_FUNCTION(native_window) {
#ifndef NO_UI
    const char *title = "";
    const char *body = "";
    if (JS_ParameterCount(ctx) >= 2) {
        JS_ParameterString(ctx, 0);
        JS_ParameterString(ctx, 1);

        body = JS_GetStringParameter(ctx, 0);
        title = JS_GetStringParameter(ctx, 1);
    } else
    if (JS_ParameterCount(ctx) >= 1) {
        JS_ParameterString(ctx, 0);
        body = JS_GetStringParameter(ctx, 0);
    }
    if (!ui_initialized) {
        ui_app_init(gui_callback);
        ui_initialized = 1;
    }

    void *window_ptr = ui_window(title, body);
    if ((ui_window_count() == 1) && (main_loop)) {
        loop_add(main_loop, ui_callback, 10, NULL);
        ui_set_event(UI_EVENT_WINDOW_CLOSE, ui_close_callback, NULL);
    }

    js_object_type obj_id = JS_NewObject(ctx, "HTMLUIWindow");
    
    JS_ObjectSetPointer(ctx, obj_id, JS_HIDDEN_SYMBOL("__native_handle"), window_ptr);
    JS_ObjectSetFunction(ctx, obj_id, "close", native_ui_window_close, 0);
    JS_ObjectSetFunction(ctx, obj_id, "minimize", native_ui_window_minimize, 0);
    JS_ObjectSetFunction(ctx, obj_id, "maximize", native_ui_window_maximize, 0);
    JS_ObjectSetFunction(ctx, obj_id, "restore", native_ui_window_restore, 0);
    JS_ObjectSetFunction(ctx, obj_id, "top", native_ui_window_top, 0);
    JS_ObjectSetFunction(ctx, obj_id, "setHtml", native_ui_window_content, JS_VARARGS);
    JS_ObjectSetFunction(ctx, obj_id, "eval", native_ui_window_js, JS_VARARGS);
    JS_ObjectSetFunction(ctx, obj_id, "call", native_ui_window_execute, JS_VARARGS);
    JS_ObjectSetFunction(ctx, obj_id, "isOpen", native_ui_window_is_opened, 0);

    char wnd_buffer[50];
    snprintf(wnd_buffer, sizeof(wnd_buffer), "wnd%X", (unsigned int)(uintptr_t)window_ptr); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(js_ctx);
    duk_dup(ctx, -2);
    duk_put_prop_string(ctx, -2, wnd_buffer);
    duk_pop(ctx);
    return 1;
#else
    JS_ObjectSetObject(ctx, global_stash(ctx), wnd_buffer, obj_id);
    JS_RETURN_OBJECT(ctx, obj_id);
#endif

#endif
	JS_RETURN_NOTHING(ctx);
}

void helper_notify(JS_CONTEXT ctx, const char *object, const char *event_name) {
#ifdef WITH_DUKTAPE
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, object);
    if (duk_is_object(ctx, -1)) {
        duk_get_prop_string(js_ctx, -1, event_name);
        if (duk_is_callable(ctx, -1))
            duk_call(ctx, 0);
        duk_pop(ctx);
    }
    duk_pop(ctx);
#else
    js_object_type obj = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), object);
    if (JS_IsObject(obj)) {
        js_object_type function_obj = JS_GetPropertyStr(ctx, obj, event_name);
        if (JS_IsFunction(ctx, function_obj))
            JS_FreeValue(ctx, JS_Call(ctx, function_obj, obj, 0, NULL));
    }
#endif
}

#ifdef WITH_DUKTAPE
static void _push_dirname(JS_CONTEXT ctx, const char *path) {
    if ((!path) || (!path[0])) {
        duk_push_string(ctx, ".");
        return;
    }
    int len = strlen(path);
    int i;
    int directory = 0;
    for (i = len - 1; i >= 0; i --) {
        if ((path[i] == '/') || (path[i] == '\\')) {
            directory = i;
            break;
        }
    }
    if (directory)
        duk_push_lstring(ctx, path, directory);
    else
        duk_push_string(ctx, ".");
}
#else
static void _push_dirname(JS_CONTEXT ctx, js_object_type obj, const char *member_name, const char *path) {
    if ((!path) || (!path[0])) {
        JS_ObjectSetString(ctx, obj, member_name, ".");
        return;
    }
    int len = strlen(path);
    int i;
    int directory = 0;
    for (i = len - 1; i >= 0; i --) {
        if ((path[i] == '/') || (path[i] == '\\')) {
            directory = i;
            break;
        }
    }
    if (directory)
        JS_ObjectSetStringLen(ctx, obj, member_name, path, directory);
    else
        JS_ObjectSetString(ctx, obj, member_name, ".");
}
#endif

#ifndef NO_IO
#ifdef WITH_DUKTAPE
static const char *duk_push_string_file_raw(JS_CONTEXT ctx, const char *path, const char *directory, js_uint_t flags) {
#else
static char *duk_push_string_file_raw(JS_CONTEXT ctx, const char *path, const char *directory) {
#endif
	FILE *f = NULL;
	char *buf;
	long sz;

	if ((!path) || (!path[0]))
		goto fail;

	f = fopen(path, "rb");
    if (!f) {
        char js_path[4096];
        snprintf(js_path, sizeof(js_path), "%s.js", path);
        f = fopen(js_path, "rb");

        if ((!f) && (directory) && (directory[0]) && (path[0] != '/') && (path[0] != '\\')) {
            snprintf(js_path, sizeof(js_path), "%s/%s", directory, path);
            f = fopen(js_path, "rb");

            if (!f) {
                snprintf(js_path, sizeof(js_path), "%s/%s.js", directory, path);
                f = fopen(js_path, "rb");
            }
        }
        if ((!f) && (directory)) {
            if ((path[0] != '.') || ((path[1] != '/') && (path[1] == '\\'))) {
                snprintf(js_path, sizeof(js_path), DEFAULT_FRAMEWORK_DIR "/%s", path);
                f = fopen(js_path, "rb");
                if (!f) {
                    snprintf(js_path, sizeof(js_path), DEFAULT_FRAMEWORK_DIR "/%s.js", path);
                    f = fopen(js_path, "rb");
                }
            }
        }
        if (f) {
#ifdef WITH_DUKTAPE
            duk_push_global_object(ctx);
                duk_get_prop_string(ctx, -1, "module");

                duk_push_string(ctx, js_path);
                duk_put_prop_string(ctx, -2, "filename");

                _push_dirname(ctx, js_path);
                duk_put_prop_string(ctx, -2, "path");
            duk_pop_2(ctx);
#else
            js_object_type obj = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "module");
            JS_ObjectSetString(ctx, obj, "filename", js_path);
            _push_dirname(ctx, obj, "path", js_path);
#endif
        }
    }
    
	if (!f)
		goto fail;

	if (fseek(f, 0, SEEK_END) < 0)
		goto fail;

	sz = ftell(f);
	if (sz < 0)
		goto fail;

	if (fseek(f, 0, SEEK_SET) < 0)
		goto fail;

#ifdef WITH_DUKTAPE
	buf = (char *) duk_push_fixed_buffer(ctx, (duk_size_t) sz);
#else
	buf = (char *) js_malloc(ctx, sz + 1);
    buf[sz] = 0;
#endif
	if ((size_t) fread(buf, 1, (size_t) sz, f) != (size_t) sz) {
#ifdef WITH_DUKTAPE
		duk_pop(ctx);
#endif
		goto fail;
	}
	(void) fclose(f);
#ifdef WITH_DUKTAPE
	return duk_buffer_to_string(ctx, -1);
#else
    return buf;
#endif

 fail:
	if (f)
		(void) fclose(f);

#ifdef WITH_DUKTAPE
	if (flags & DUK_STRING_PUSH_SAFE)
		duk_push_undefined(ctx);
	else
#endif
        JS_Error(ctx, "read file error");

	return NULL;
}

void duk_eval_file(JS_CONTEXT ctx, const char *path, const char *directory) {
#ifdef WITH_DUKTAPE
	duk_push_string_file_raw(ctx, path, directory, DUK_STRING_PUSH_SAFE);
	duk_push_string(ctx, path);
	duk_compile(ctx, DUK_COMPILE_EVAL | DUK_COMPILE_SHEBANG);
	duk_push_global_object(ctx); 
	duk_call_method(ctx, 0);
#else
    char *buf = duk_push_string_file_raw(ctx, path, directory);
    if (buf) {
        JS_EvalSimplePath(ctx, buf, path);
        js_free(ctx, buf);
    }
#endif
}
#endif

static int builtin_module_lookup(JS_CONTEXT ctx, const char *path) {
    int module_index = 0;
    struct builtin_module bmodule = builtin_modules[module_index ++];
    int found = 0;
    while (bmodule.name) {
        if (!strcmp(bmodule.name, path)) {
#ifndef NO_COMPRESSION
            if ((bmodule.compressed) && (bmodule.size > 0)) {
                mz_ulong max_len = bmodule.original_size;
                if (!max_len)
                    max_len = bmodule.size * 4 + 0x100;

                max_len ++;
                char *uncompressed = (char *)malloc(max_len);
                if (!uncompressed) {
                    JS_Error(ctx, "cannot allocate memory");
                    break;
                }
                int err = mz_uncompress((unsigned char *)uncompressed, &max_len, (unsigned char *)bmodule.data, bmodule.size);
                if (!err) {
                    uncompressed[max_len] = 0;
                    JS_Eval(ctx, uncompressed, bmodule.name);
                    found = 1;
                } else {
                    JS_Error(ctx, mz_error(err));
                    break;
                }
                free(uncompressed);
                break;
            } else
#endif
            JS_Eval(ctx, bmodule.data, bmodule.name);
            found = 1;
            break;
        }
        bmodule = builtin_modules[module_index ++];
    }
    return found;
}

JS_C_FUNCTION(native_require) {
    JS_ParameterString(ctx, 0);

    const char *module_id = JS_GetStringParameter(ctx, 0);

#ifdef WITH_DUKTAPE
    if (module_id) {
        duk_push_heap_stash(js_ctx);
        duk_get_prop_string(ctx, -1, "loaded_modules");
        duk_get_prop_string(ctx, -1, module_id);
        if (!duk_is_undefined(ctx, -1)) {
            duk_get_prop_string(ctx, -1, "exports");
            // duk_dup(ctx, -1);
            // duk_pop(js_ctx);
            return 1;
        }
        duk_pop_3(js_ctx);
    }

    duk_push_global_object(ctx); 

    char *path = NULL;
    duk_get_prop_string(ctx, -1, "module");
        duk_get_prop_string(ctx, -1, "path");
        const char *parent_path = duk_safe_to_string(ctx, -1);
        if (parent_path)
            path = strdup(parent_path);
        duk_pop(ctx);

        duk_push_heap_stash(js_ctx);
            duk_dup(ctx, -2);
            duk_put_prop_string(ctx, -2, "parent_module");
        duk_pop(ctx);
    duk_pop(ctx);

    const char *filename = duk_safe_to_string(ctx, 0);

    duk_get_global_string(ctx, "Module");
    duk_new(ctx, 0);
    duk_idx_t obj_idx = duk_get_top(ctx) - 1;

        duk_dup(ctx, -3);
        duk_put_prop_string(ctx, -2, "id");

        duk_dup(ctx, -3);
        duk_put_prop_string(ctx, -2, "filename");

        _push_dirname(ctx, filename);
        duk_put_prop_string(ctx, -2, "path");

        duk_push_heap_stash(js_ctx);
            duk_get_prop_string(ctx, -1, "parent_module");
            duk_dup(ctx, -1);
            duk_put_prop_string(ctx, obj_idx, "parent");

            duk_del_prop_string(ctx, -2, "parent_module");
        duk_pop_2(ctx);

        duk_push_boolean(ctx, 0);
        duk_put_prop_string(ctx, -2, "loaded");

        duk_push_array(ctx);
        duk_put_prop_string(ctx, -2, "children");

        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "exports");

        duk_push_c_function(ctx, native_require, 1);
        duk_put_prop_string(ctx, -2, "require");

        if (module_id) {
            duk_push_heap_stash(js_ctx);
                duk_get_prop_string(ctx, -1, "loaded_modules");
                duk_dup(ctx, obj_idx);
                duk_put_prop_string(ctx, -2, module_id);
            duk_pop_2(ctx);
        }

        duk_put_prop_string(ctx, -2, "module");
    duk_pop(ctx);

    duk_push_global_object(ctx); 
        duk_get_prop_string(ctx, -1, "module");
        duk_get_prop_string(ctx, -1, "exports");
        duk_put_prop_string(ctx, -3, "exports");
    duk_pop_2(ctx);

    if (!builtin_module_lookup(ctx, module_id)) {
#ifdef NO_IO
        JS_Error(ctx, "no sourcecode");
#else
        duk_eval_file(ctx, filename, path ? path : "");
        duk_pop(ctx);
#endif
    }

    if (path)
        free(path);

    duk_push_global_object(ctx); 
        duk_get_prop_string(ctx, -1, "module");
        duk_get_prop_string(ctx, -1, "exports");
        // leave value on the stack

    duk_push_global_object(ctx); 
        duk_get_prop_string(ctx, -1, "module");
            duk_push_boolean(ctx, 1);
            duk_put_prop_string(ctx, -2, "loaded");
            duk_get_prop_string(ctx, -1, "parent");
                int arr_len = 0;
                duk_get_prop_string(ctx, -1, "children");
                    duk_get_prop_string(ctx, -1, "length");
                        arr_len = duk_to_int(ctx, -1);
                    duk_pop(ctx);
                    duk_dup(ctx, -3);
                    duk_put_prop_index(ctx, -2, (duk_uarridx_t)arr_len);
                duk_pop(ctx);

                // exports = parent.exports
                duk_get_prop_string(ctx, -1, "exports");
                duk_put_prop_string(ctx, -4, "exports");

            duk_put_prop_string(ctx, -3, "module");
        duk_pop(ctx);
    duk_pop(ctx);

    // JS_EvalSimple(ctx, "module.loaded = true; if (module.parent) module.parent.children.push(module); module = module.parent;");

    return 1;
#else
    js_object_type loaded_modules = JS_GetPropertyStr(ctx, global_stash(ctx), "loaded_modules");
    if (module_id) {
        js_object_type mod = JS_GetPropertyStr(ctx, loaded_modules, module_id);
        if (!JS_IsUndefined(mod))
            return JS_GetPropertyStr(ctx, mod, "exports");
    }

    js_object_type parent_module = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "module");

    const char *filename = JS_GetStringParameter(ctx, 0);
    js_object_type module = JS_NewObject(ctx, "Module");
    
    JS_ObjectSetString(ctx, module, "id", module_id);
    JS_ObjectSetString(ctx, module, "filename", module_id);
    _push_dirname(ctx, module, "path", module_id);
    JS_ObjectSetObject(ctx, module, "parent", parent_module);

    JS_ObjectSetBoolean(ctx, module, "loaded", 0);
    JS_SetPropertyStr(ctx, module, "children", JS_NewArray(ctx));
    js_object_type exports_obj = JS_NewPlainObject(ctx);
    JS_ObjectSetObject(ctx, module, "exports", exports_obj);   
    JS_ObjectSetFunction(ctx, module, "require", native_require, 1);

    JS_ObjectSetObject(ctx, JS_GetGlobalObject(ctx), "exports", exports_obj);

    JS_ObjectSetObject(ctx, loaded_modules, module_id, module);

    JS_ObjectSetObject(ctx, JS_GetGlobalObject(ctx), "module", module);
    if (!builtin_module_lookup(ctx, module_id)) {
#ifdef NO_IO
        JS_Error(ctx, "no sourcecode");
#else
        const char *path = JS_ToCString(ctx, JS_GetPropertyStr(ctx, parent_module, "path"));
        duk_eval_file(ctx, filename, path ? path : "");
#endif
    }
    JS_ObjectSetBoolean(ctx, module, "loaded", 1);
   
    JS_SetPropertyUint32(ctx, JS_GetPropertyStr(ctx, parent_module, "children"), _JS_GetIntParameter(ctx, JS_GetPropertyStr(ctx, parent_module, "length")), module);

    JS_ObjectSetObject(ctx, JS_GetGlobalObject(ctx), "module", parent_module);
    JS_ObjectSetObject(ctx, JS_GetGlobalObject(ctx), "exports", JS_GetPropertyStr(ctx, parent_module, "exports"));

    return JS_GetPropertyStr(ctx, module, "exports");
#endif
}

#ifdef _WIN32
static int has_ticks = 0;

static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks) {
    static unsigned long long _previousTotalTicks = 0;
    static unsigned long long _previousIdleTicks = 0;

    unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
    unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

    float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

    if ((_previousTotalTicks) && (_previousTotalTicks != totalTicks))
        has_ticks = 1;
    else
        has_ticks = 0;
    _previousTotalTicks = totalTicks;
    _previousIdleTicks = idleTicks;
    return ret;
}

static unsigned long long FileTimeToInt64(const FILETIME *ft) {
    return (((unsigned long long)(ft->dwHighDateTime)) << 32) | ((unsigned long long)ft->dwLowDateTime);
}

float GetCPULoad() {
    FILETIME idleTime, kernelTime, userTime;
    return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(&idleTime), FileTimeToInt64(&kernelTime) + FileTimeToInt64(&userTime)) : -1.0f;
}
#endif

JS_C_FUNCTION(cpuUsage) {
#ifdef _WIN32
    double usage = GetCPULoad();
    if (!has_ticks) {
        Sleep(1);
        usage = GetCPULoad();
    }
    JS_RETURN_NUMBER(ctx, usage);
#else
#if defined(__FreeBSD__) || defined(__APPLE__)
   int ncpu;
   size_t len = sizeof(int);
   if (sysctlbyname("hw.ncpu", &ncpu, &len, NULL, 0) == -1 || !len)
        ncpu = 1;
   FILE *fp = popen("ps -o %cpu -p `pgrep -S idle`", "r");
   if (fp) {
       double usage = -1.0f;
       fscanf(fp," %*s\n%lf", &usage);
       fclose(fp);
       if (usage > 0) {
           usage /= ncpu;
           usage = (100.0f - usage) / 100;
       }
       JS_RETURN_NUMBER(ctx, usage);
   } else {
       JS_RETURN_NUMBER(ctx, -1);
   }
#else
#ifdef __linux__
    static double abase = 0.0f;
    static double abase2 = 0.0f;
    static time_t last_read = 0;
    double a[4], b[4], loadavg;
    FILE *fp = fopen("/proc/stat","r");
    if (fp) {
        if (!last_read) {
            fscanf(fp,"%*s %lf %lf %lf %lf", &a[0], &a[1], &a[2], &a[3]);
            fclose(fp);
            usleep(1000);

            fp = fopen("/proc/stat","r");
            abase = a[0] + a[1] + a[2] + a[3];
            abase2 = a[0] + a[1] + a[2];
            last_read = time(NULL);
        }
        fscanf(fp,"%*s %lf %lf %lf %lf", &b[0], &b[1], &b[2], &b[3]);
        fclose(fp);

        double ab = (b[0]+b[1]+b[2]+b[3]) - (abase);
        if (ab != 0.0f)
            loadavg = ((b[0]+b[1]+b[2]) - (abase2)) / ab;
        else
            loadavg = 0;

        if ((time(NULL) - last_read) >= 2) {
            abase = b[0] + b[1] + b[2] + b[3];
            abase2 = b[0] + b[1] + b[2];
            last_read = time(NULL);
        }
        JS_RETURN_NUMBER(ctx, loadavg);
    } else {
        JS_RETURN_UNDEFINED(ctx);
    }
#else
    JS_RETURN_UNDEFINED(ctx);
#endif
#endif
#endif
}

#ifdef _WIN32
BOOL sig_handler(DWORD fdwCtrlType) {
    char buf[0x80];
    int sig = 15;
    snprintf(buf, sizeof(buf), "if ((process._sig_handler) && (process._sig_handler[%i])) process._sig_handler[%i](%i);", sig, sig, sig);
    JS_EvalSimple(js(), buf);
    return TRUE;
}
#else
void sig_handler(int sig) {
    char buf[0x80];
    snprintf(buf, sizeof(buf), "if ((process._sig_handler) && (process._sig_handler[%i])) process._sig_handler[%i](%i);", sig, sig, sig);
    JS_EvalSimple(js(), buf);
}
#endif

JS_C_FUNCTION(js_signal) {
    JS_ParameterNumber(ctx, 0);
#ifdef _WIN32
    int signal_num = JS_GetIntParameter(ctx, 0);
    // just SIG_TERM supported
    if (signal_num == 15) {
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)sig_handler, TRUE);
        JS_RETURN_NUMBER(ctx, 0);
    }
    JS_RETURN_NUMBER(ctx, -1);
#else
#ifdef ESP32
    JS_RETURN_NUMBER(ctx, -1);
#else
    int signal_num = JS_GetIntParameter(ctx, 0);
    int err = 0;
    switch (signal_num) {
        case 1:
            signal(SIGHUP, sig_handler);
            break;
        case 2:
            signal(SIGINT, sig_handler);
            break;
        case 3:
            signal(SIGQUIT, sig_handler);
            break;
        case 4:
            signal(SIGILL, sig_handler);
            break;
        case 5:
            signal(SIGTRAP, sig_handler);
            break;
        case 6:
#if defined(__FreeBSD__) || defined(__APPLE__)
            signal(SIGIOT, sig_handler);
#else
            signal(SIGABRT, sig_handler);
#endif
            break;
        case 7:
            signal(SIGBUS, sig_handler);
            break;
        case 8:
            signal(SIGFPE, sig_handler);
            break;
        case 9:
            signal(SIGKILL, sig_handler);
            break;
        case 10:
            signal(SIGUSR1, sig_handler);
            break;
        case 11:
            signal(SIGSEGV, sig_handler);
            break;
        case 12:
            signal(SIGUSR2, sig_handler);
            break;
        case 13:
            signal(SIGPIPE, sig_handler);
            break;
        case 14:
            signal(SIGALRM, sig_handler);
            break;
        case 15:
            signal(SIGTERM, sig_handler);
            break;
#ifdef SIGSTKFLT
        case 16:
            signal(SIGSTKFLT, sig_handler);
            break;
#endif
        case 17:
            signal(SIGCHLD, sig_handler);
            break;
        case 18:
            signal(SIGCONT, sig_handler);
            break;
        case 19:
            signal(SIGSTOP, sig_handler);
            break;
        case 20:
            signal(SIGTSTP, sig_handler);
            break;
        case 21:
            signal(SIGTTIN, sig_handler);
            break;
        case 22:
            signal(SIGTTOU, sig_handler);
            break;
        case 23:
            signal(SIGURG, sig_handler);
            break;
        case 24:
            signal(SIGXCPU, sig_handler);
            break;
        case 25:
            signal(SIGXFSZ, sig_handler);
            break;
        case 26:
            signal(SIGVTALRM, sig_handler);
            break;
        case 27:
            signal(SIGPROF, sig_handler);
            break;
        case 28:
            signal(SIGWINCH, sig_handler);
            break;
        case 29:
            signal(SIGIO, sig_handler);
            break;
#ifdef SIGPWR
        case 30:
            signal(SIGPWR, sig_handler);
            break;
#endif
        case 31:
            signal(SIGSYS, sig_handler);
            break;
        default:
            err = -1;
    }
    JS_RETURN_NUMBER(ctx, err);
#endif
#endif
}

JS_C_FUNCTION(js_kill) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    int err = -1;
#ifndef _WIN32
    #ifndef ESP32
        switch (JS_GetIntParameter(ctx, 1)) {
            case 1:
                err = kill(JS_GetIntParameter(ctx, 0), SIGHUP);
                break;
            case 2:
                err = kill(JS_GetIntParameter(ctx, 0), SIGINT);
                break;
            case 3:
                err = kill(JS_GetIntParameter(ctx, 0), SIGQUIT);
                break;
            case 4:
                err = kill(JS_GetIntParameter(ctx, 0), SIGILL);
                break;
            case 5:
                err = kill(JS_GetIntParameter(ctx, 0), SIGTRAP);
                break;
            case 6:
    #if defined(__FreeBSD__) || defined(__APPLE__)
                err = kill(JS_GetIntParameter(ctx, 0), SIGIOT);
    #else
                err = kill(JS_GetIntParameter(ctx, 0), SIGABRT);
    #endif
                break;
            case 7:
                err = kill(JS_GetIntParameter(ctx, 0), SIGBUS);
                break;
            case 8:
                err = kill(JS_GetIntParameter(ctx, 0), SIGFPE);
                break;
            case 9:
                err = kill(JS_GetIntParameter(ctx, 0), SIGKILL);
                break;
            case 10:
                err = kill(JS_GetIntParameter(ctx, 0), SIGUSR1);
                break;
            case 11:
                err = kill(JS_GetIntParameter(ctx, 0), SIGSEGV);
                break;
            case 12:
                err = kill(JS_GetIntParameter(ctx, 0), SIGUSR2);
                break;
            case 13:
                err = kill(JS_GetIntParameter(ctx, 0), SIGPIPE);
                break;
            case 14:
                err = kill(JS_GetIntParameter(ctx, 0), SIGALRM);
                break;
            case 15:
                err = kill(JS_GetIntParameter(ctx, 0), SIGTERM);
                break;
#ifdef SIGSTKFLT
            case 16:
                err = kill(JS_GetIntParameter(ctx, 0), SIGSTKFLT);
                break;
#endif
            case 17:
                err = kill(JS_GetIntParameter(ctx, 0), SIGCHLD);
                break;
            case 18:
                err = kill(JS_GetIntParameter(ctx, 0), SIGCONT);
                break;
            case 19:
                err = kill(JS_GetIntParameter(ctx, 0), SIGSTOP);
                break;
            case 20:
                err = kill(JS_GetIntParameter(ctx, 0), SIGTSTP);
                break;
            case 21:
                err = kill(JS_GetIntParameter(ctx, 0), SIGTTIN);
                break;
            case 22:
                err = kill(JS_GetIntParameter(ctx, 0), SIGTTOU);
                break;
            case 23:
                err = kill(JS_GetIntParameter(ctx, 0), SIGURG);
                break;
            case 24:
                err = kill(JS_GetIntParameter(ctx, 0), SIGXCPU);
                break;
            case 25:
                err = kill(JS_GetIntParameter(ctx, 0), SIGXFSZ);
                break;
            case 26:
                err = kill(JS_GetIntParameter(ctx, 0), SIGVTALRM);
                break;
            case 27:
                err = kill(JS_GetIntParameter(ctx, 0), SIGPROF);
                break;
            case 28:
                err = kill(JS_GetIntParameter(ctx, 0), SIGWINCH);
                break;
            case 29:
                err = kill(JS_GetIntParameter(ctx, 0), SIGIO);
                break;
#ifdef SIGPWR
            case 30:
                err = kill(JS_GetIntParameter(ctx, 0), SIGPWR);
                break;
#endif
            case 31:
                err = kill(JS_GetIntParameter(ctx, 0), SIGSYS);
                break;
        }
    #endif
#endif
    JS_RETURN_NUMBER(ctx, err);
}

void duk_run_file(JS_CONTEXT ctx, const char *path) {
#ifdef WITH_DUKTAPE
    duk_push_global_object(ctx); 
    // duk_push_object(ctx);
    JS_NewObject(ctx, "Module");

        duk_push_string(ctx, path);
        duk_put_prop_string(ctx, -2, "id");

        duk_push_string(ctx, path);
        duk_put_prop_string(ctx, -2, "filename");

        _push_dirname(ctx, path);
        duk_put_prop_string(ctx, -2, "path");

        duk_push_null(ctx);
        duk_put_prop_string(ctx, -2, "parent");

        duk_push_boolean(ctx, 0);
        duk_put_prop_string(ctx, -2, "loaded");

        duk_push_array(ctx);
        duk_put_prop_string(ctx, -2, "children");

        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "exports");

        duk_get_prop_string(ctx, -1, "exports");
        duk_put_prop_string(ctx, -3, "exports");

        duk_push_c_function(ctx, native_require, 1);
        duk_put_prop_string(ctx, -2, "require");

	    duk_put_prop_string(ctx, -2, "module");
    duk_pop(ctx);
#else
    js_object_type obj = JS_NewObject(ctx, "Module");
    JS_ObjectSetObject(ctx, JS_GetGlobalObject(ctx), "module", obj);

    JS_ObjectSetString(ctx, obj, "id", path);
    JS_ObjectSetString(ctx, obj, "filename", path);
    _push_dirname(ctx, obj, "path", path);
    JS_ObjectSetNull(ctx, obj, "parent");
    JS_ObjectSetBoolean(ctx, obj, "loaded", 0);
    JS_SetPropertyStr(ctx, obj, "children", JS_NewArray(ctx));
    js_object_type exports_obj = JS_NewPlainObject(ctx);
    JS_ObjectSetObject(ctx, obj, "exports", exports_obj);   
    JS_ObjectSetFunction(ctx, obj, "require", native_require, 1);

    JS_ObjectSetObject(ctx, JS_GetGlobalObject(ctx), "exports", exports_obj);
#endif

    if (!builtin_module_lookup(ctx, path)) {
#ifdef NO_IO
        JS_Error(ctx, "no sourcecode");
#else
        duk_eval_file(ctx, path, NULL);
#endif
    }
}

JS_CONTEXT js() {
    return js_ctx;
}

struct doops_loop *js_loop() {
    return main_loop;
}

void register_builtins(struct doops_loop *loop, JS_CONTEXT ctx, int argc, char *argv[], char *envp[]) {
    int i;

    js_ctx = ctx;
    main_loop = loop;

#ifndef _WIN32
    #ifndef ESP32
        signal(SIGPIPE, SIG_IGN);
    #endif
#endif

#ifdef WITH_DUKTAPE
    duk_push_heap_stash(js_ctx);
        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "loaded_modules");
    duk_pop(js_ctx);
#else
    JS_SetPropertyStr(js_ctx, global_stash(ctx), "loaded_modules", JS_NewPlainObject(ctx));
#endif

    register_global_function(ctx, "setInterval", setInterval, 2);
    register_global_function(ctx, "setTimeout", setTimeout, 2);
    register_global_function(ctx, "setImmediate", setImmediate, 1);

    register_global_function(ctx, "clearInterval", clearInterval, 1);
    register_global_function(ctx, "clearTimeout", clearTimeout, 1);
    register_global_function(ctx, "clearImmediate", clearImmediate, 1);
#ifndef NO_UI
    register_global_function(ctx, "alert", native_alert, JS_VARARGS);
#else
    register_global_function(ctx, "alert", native_log, JS_VARARGS);
#endif
#ifndef NO_IO
    register_io_functions(loop, ctx);
#endif
#ifndef NO_SOCKET
    register_socket_functions(loop, ctx);
#endif
#ifdef WITH_GPIO
    register_gpio_functions(loop, ctx);
#endif
#ifdef WITH_NVS
    register_nvs_functions(loop, ctx);
#endif
#ifdef WITH_WIFI
    register_wifi_functions(loop, ctx);
#endif
#ifdef ESP32
    register_esp32_functions(loop, ctx);
#endif
    register_global_function(ctx, "require", native_require, 1);
    register_object(ctx, "console", "log", native_log, "warn", native_warning, "error", native_error, (void *)NULL);
    register_object(ctx, "app", "quit", native_quit, "exit", native_exit, "window", native_window, (void *)NULL);
#ifdef WITH_DUKTAPE
    duk_push_global_object(ctx);
#endif

#ifndef NO_HTTP
    register_object(ctx, "_http_helpers", "parseRequest", parseRequest, "parseResponse", parseResponse, (void *)NULL);
#endif
    
    register_object(ctx, "process", "abort", native_quit, "exit", native_exit, "chdir", native_chdir, "cwd", native_cwd, "nextTick", setImmediate, "randomBytes", randomBytes, "cpuUsage", cpuUsage, "mapSignal", js_signal, "kill", js_kill, (void *)NULL);
#ifdef WITH_DUKTAPE
    duk_push_string(ctx, "process");
    duk_get_prop(ctx, -2);

    duk_idx_t arr_idx = duk_push_array(ctx);
    for (i = 0; i < argc; i ++) {
        duk_push_string(ctx, argv[i]);
        duk_put_prop_index(ctx, arr_idx, i);
    }
    duk_put_prop_string(ctx, -2, "argv");

    arr_idx = duk_push_array(ctx);
    for (i = 2; i < argc; i ++) {
        duk_push_string(ctx, argv[i]);
        duk_put_prop_index(ctx, arr_idx, i - 2);
    }

    duk_put_prop_string(ctx, -2, "execArgv");

    if ((argv) && (argv[0])) {
        duk_push_string(ctx, argv[0]);
        duk_put_prop_string(ctx, -2, "execPath");
    }

#ifdef _WIN32
    duk_push_string(ctx, "win32");
#else
#ifdef ESP32
    duk_push_string(ctx, "esp");
#else
    duk_push_string(ctx, "unix");
#endif
#endif
    duk_put_prop_string(ctx, -2, "platform");

    duk_idx_t obj_idx = duk_push_object(ctx);
    if (envp) {
        i = 0;
        char *env = envp[i ++];
        while (env) {
            char *var = strstr(env, "=");
            if ((var) && (var != env)) {
                duk_push_string(ctx, var + 1);
                duk_put_prop_lstring(ctx, obj_idx, env, (int)((uintptr_t)var - (uintptr_t)env));
            }
            env = envp[i ++];
        }
    }
    duk_put_prop_string(ctx, -2, "env");

    duk_push_string(ctx, "v1.0.0");
    duk_put_prop_string(ctx, -2, "version");
#else
    js_object_type process = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "process");
    js_object_type arr = JS_NewArray(ctx);
    JS_SetPropertyStr(ctx, process, "argv", arr);
    js_object_type arrExec = JS_NewArray(ctx);
    JS_SetPropertyStr(ctx, process, "execArgv", arrExec);

    for (i = 0; i < argc; i ++) {
        JS_SetPropertyUint32(ctx, arr, (unsigned int)i, JS_NewString(ctx, argv[i]));
        if (i > 2)
            JS_SetPropertyUint32(ctx, arrExec, (unsigned int)i - 2, JS_NewString(ctx, argv[i]));
    }

    if ((argv) && (argv[0]))
        JS_ObjectSetString(ctx, process, "execPath", argv[0]);

#ifdef _WIN32
    JS_ObjectSetString(ctx, process, "platform", "win32");
#else
#ifdef ESP32
    JS_ObjectSetString(ctx, process, "platform", "esp");
#else
    JS_ObjectSetString(ctx, process, "platform", "v1.0.0");
#endif
#endif
    js_object_type arrEnv = JS_NewPlainObject(ctx);
    if (envp) {
        i = 0;
        char *env = envp[i ++];
        while (env) {
            char *var = strstr(env, "=");
            if ((var) && (var != env))
                JS_ObjectSetStringLenLen(ctx, arrEnv, env, (int)((uintptr_t)var - (uintptr_t)env), var + 1, strlen(var + 1));
            env = envp[i ++];
        }
    }
    JS_SetPropertyStr(ctx, process, "env", arrEnv);

    JS_ObjectSetString(ctx, process, "version", "unix");
#endif

    JS_EvalSimple(ctx, "process.constants = {SIGHUP: 1, SIGINT: 2, SIGQUIT: 3, SIGILL: 4, SIGTRAP: 5, SIGABRT: 6, SIGBUS:  7, SIGFPE: 8, SIGKILL: 9, SIGUSR1: 10, SIGSEGV: 11, SIGUSR2: 12, SIGPIPE: 13, SIGALRM: 14, SIGTERM: 15, "
                                            "SIGSTKFLT: 16, SIGCHLD: 17, SIGCONT: 18, SIGSTOP: 19, SIGTSTP: 20, SIGTTIN: 21, SIGTTOU: 22, SIGURG: 23, SIGXCPU: 24, SIGXFSZ: 25, SIGVTALRM: 26, SIGPROF: 28, SIGIO: 29, SIGPWR: 30, SIGSYS: 31};"
                                            "process.on = function(sig, handler) { if (typeof sig == 'string') sig = process.constants[sig]; if (process.mapSignal(sig)) return; if (!process._sig_handler) process._sig_handler = [ ]; process._sig_handler[sig] = handler; };"
                                            "process.signal = process.on;"
    );
#ifdef WITH_DUKTAPE
    duk_pop_2(ctx);
#endif
    JS_EvalSimple(ctx, JS_GLOBAL);
#ifdef WITH_DUKTAPE
    JS_EvalSimple(ctx, "global.gc = Duktape.gc; global.__destructor = Duktape.fin;");
#else
    JS_EvalSimple(ctx, "global.gc = std.gc; global.__destructor = function() { /* to do */ };");
#endif
    JS_EvalSimple(ctx, JS_MODULE);
    JS_EvalSimple(ctx, JS_WINDOW);
    JS_EvalSimple(ctx, JS_PROMISE);
    JS_EvalSimple(ctx, JS_CONSOLE);
#ifndef NO_IO
    JS_EvalSimple(ctx, JS_STATS);
#endif
#ifndef WITH_GPIO
    JS_EvalSimple(ctx, JS_GPIO_DUMMY);
#endif
}