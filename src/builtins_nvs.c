#include "builtins.h"
#include "builtins_nvs.h"

#ifdef ESP32
    #include <nvs_flash.h>
#else
    #include "misc/lmdb/lmdb.h"
    #include "misc/lmdb/mdb.c"
    #include "misc/lmdb/midl.c"
#endif

#define STR_HELPER(x)   #x
#define TO_STR(x)       STR_HELPER(x)

#ifdef ESP32
static int nvs_initialized = 0;
#endif

JS_C_FUNCTION(js_nvs_flash_init) {
#ifdef ESP32
    if (nvs_initialized) {
        JS_RETURN_NUMBER(ctx, 0);
    }
    esp_err_t ret = nvs_flash_init();
    if (!ret)
        nvs_initialized = 1;
    JS_RETURN_NUMBER(ctx, ret);
#else
    JS_RETURN_NUMBER(ctx, 0);
#endif
}

JS_C_FUNCTION(js_nvs_flash_deinit) {
#ifdef ESP32
    esp_err_t ret = nvs_flash_deinit();
    if (!ret)
        nvs_initialized = 0;
    JS_RETURN_NUMBER(ctx, ret);
#else
    JS_RETURN_NUMBER(ctx, 0);
#endif
}

JS_C_FUNCTION(js_nvs_flash_erase) {
#ifdef ESP32
    if ((!nvs_initialized) && (!nvs_flash_init()))
        nvs_initialized = 1;
    esp_err_t ret = nvs_flash_erase();
    JS_RETURN_NUMBER(ctx, ret);
#else
    JS_RETURN_NUMBER(ctx, 0);
#endif
}

#ifdef ESP32
JS_C_FUNCTION(js_nvs_flash_erase_partition) {
    JS_ParameterString(ctx, 0);
    if ((!nvs_initialized) && (!nvs_flash_init()))
        nvs_initialized = 1;
    const char *partition = JS_GetStringParameter(ctx, 0);
    esp_err_t ret = nvs_flash_erase_partition(partition);
    JS_FreeString(ctx, partition);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_flash_init_partition) {
    JS_ParameterString(ctx, 0);
    if ((!nvs_initialized) && (!nvs_flash_init()))
        nvs_initialized = 1;
    const char *partition = JS_GetStringParameter(ctx, 0);
    esp_err_t ret = nvs_flash_init_partition(partition);
    JS_FreeString(ctx, partition);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_open_from_partition) {
    JS_ParameterString(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    if ((!nvs_initialized) && (!nvs_flash_init()))
        nvs_initialized = 1;
    nvs_handle out_handle = NULL;
    const char *s0 = JS_GetStringParameter(ctx, 0);
    const char *s1 = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_open_from_partition(s0, s1, JS_GetIntParameter(ctx, 2), &out_handle);
    JS_FreeString(ctx, s0);
    JS_FreeString(ctx, s1);
    if (!ret)
        JS_RETURN_POINTER(ctx, (void *)out_handle);
    JS_RETURN_UNDEFINED(ctx);
}
#endif

JS_C_FUNCTION(js_nvs_open) {
    JS_ParameterString(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    const char *str = JS_GetStringParameter(ctx, 0);
#ifdef ESP32
    if ((!nvs_initialized) && (!nvs_flash_init()))
        nvs_initialized = 1;
    nvs_handle out_handle = NULL;
    esp_err_t ret = nvs_open(str, JS_GetIntParameter(ctx, 1), &out_handle);
#else
    MDB_env *out_handle;
    int mode = 0664;
    mdb_env_create(&out_handle);
    int flags = MDB_RDONLY;
    if (JS_GetIntParameter(ctx, 1))
        flags = 0;
    int ret = mdb_env_open(out_handle, str, flags, mode);
    if ((ret) && (flags != MDB_RDONLY)) {
#ifdef _WIN32
        if (!mkdir(str))
#else
        if (!mkdir(str, mode))
#endif
            ret = mdb_env_open(out_handle, str, flags, mode);
    }
    if (ret)
        mdb_env_close(out_handle);
#endif
    JS_FreeString(ctx, str);
    if (!ret)
        JS_RETURN_POINTER(ctx, (void *)out_handle);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_nvs_close) {
    JS_ParameterPointer(ctx, 0);
#ifdef ESP32
    nvs_close((nvs_handle)JS_GetPointerParameter(ctx, 0));
#else
    mdb_env_close((MDB_env *)JS_GetPointerParameter(ctx, 0));
#endif
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_nvs_commit) {
    JS_ParameterPointer(ctx, 0);
#ifdef ESP32
    esp_err_t ret = nvs_commit((nvs_handle)JS_GetPointerParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, ret);
#else
    JS_RETURN_NUMBER(ctx, 0);
#endif
}

#ifdef ESP32
JS_C_FUNCTION(js_nvs_set_u8) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_set_u8((nvs_handle)JS_GetPointerParameter(ctx, 0), key, (unsigned char)JS_GetIntParameter(ctx, 2));
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_set_i16) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_set_i16((nvs_handle)JS_GetPointerParameter(ctx, 0), key, (short)JS_GetIntParameter(ctx, 2));
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_set_u16) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_set_u16((nvs_handle)JS_GetPointerParameter(ctx, 0), key, (unsigned short)JS_GetIntParameter(ctx, 2));
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_set_i32) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_set_i32((nvs_handle)JS_GetPointerParameter(ctx, 0), key, (int)JS_GetIntParameter(ctx, 2));
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_set_u32) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_set_u32((nvs_handle)JS_GetPointerParameter(ctx, 0), key, (unsigned int)JS_GetNumberParameter(ctx, 2));
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_set_i64) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_set_i64((nvs_handle)JS_GetPointerParameter(ctx, 0), key, (int64_t)JS_GetNumberParameter(ctx, 2));
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_set_u64) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_set_u64((nvs_handle)JS_GetPointerParameter(ctx, 0), key, (uint64_t)JS_GetNumberParameter(ctx, 2));
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_get_u8) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    uint8_t out_value = 0;
    esp_err_t ret = nvs_get_u8((nvs_handle)JS_GetPointerParameter(ctx, 0), key, &out_value);
    JS_FreeString(ctx, key);
    if (ret)  {
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_NUMBER(ctx, out_value);
}

JS_C_FUNCTION(js_nvs_get_i16) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    int16_t out_value = 0;
    esp_err_t ret = nvs_get_i16((nvs_handle)JS_GetPointerParameter(ctx, 0), key, &out_value);
    JS_FreeString(ctx, key);
    if (ret)  {
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_NUMBER(ctx, out_value);
}

JS_C_FUNCTION(js_nvs_get_u16) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    uint16_t out_value = 0;
    esp_err_t ret = nvs_get_u16((nvs_handle)JS_GetPointerParameter(ctx, 0), key, &out_value);
    JS_FreeString(ctx, key);
    if (ret)  {
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_NUMBER(ctx, out_value);
}

JS_C_FUNCTION(js_nvs_get_i32) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    int32_t out_value = 0;
    esp_err_t ret = nvs_get_i32((nvs_handle)JS_GetPointerParameter(ctx, 0), key, &out_value);
    JS_FreeString(ctx, key);
    if (ret)  {
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_NUMBER(ctx, out_value);
}

JS_C_FUNCTION(js_nvs_get_u32) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    uint32_t out_value = 0;
    esp_err_t ret = nvs_get_u32((nvs_handle)JS_GetPointerParameter(ctx, 0), key, &out_value);
    JS_FreeString(ctx, key);
    if (ret)  {
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_NUMBER(ctx, out_value);
}

JS_C_FUNCTION(js_nvs_get_i64) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    int64_t out_value = 0;
    esp_err_t ret = nvs_get_i64((nvs_handle)JS_GetPointerParameter(ctx, 0), key, &out_value);
    JS_FreeString(ctx, key);
    if (ret)  {
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_NUMBER(ctx, out_value);
}

JS_C_FUNCTION(js_nvs_get_u64) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    uint64_t out_value = 0;
    esp_err_t ret = nvs_get_u64((nvs_handle)JS_GetPointerParameter(ctx, 0), key, &out_value);
    JS_FreeString(ctx, key);
    if (ret)  {
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_NUMBER(ctx, out_value);
}
#endif

JS_C_FUNCTION(js_nvs_set_str) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterString(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    const char *str = JS_GetStringParameter(ctx, 2);
#ifdef ESP32
    esp_err_t ret = nvs_set_str((nvs_handle)JS_GetPointerParameter(ctx, 0), key, str);
#else
    MDB_txn *txn = NULL;
    MDB_dbi dbi;
    MDB_env *env = (MDB_env *)JS_GetPointerParameter(ctx, 0);

    MDB_val db_key, data;

    db_key.mv_size = strlen(key);
    db_key.mv_data = (void *)key;

    data.mv_size = strlen(str);
    data.mv_data = (void *)str;

    mdb_txn_begin(env, NULL, 0, &txn);
    mdb_open(txn, NULL, 0, &dbi);

    int ret = mdb_put(txn, dbi, &db_key, &data, 0);

    mdb_txn_commit(txn);
    mdb_close(env, dbi);
#endif
    JS_FreeString(ctx, key);
    JS_FreeString(ctx, str);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_get_str) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    size_t required_size = JS_GetIntParameter(ctx, 2);
    required_size ++;
#ifdef ESP32
    char *buf = (char *)malloc(required_size);
    esp_err_t ret = nvs_get_str((nvs_handle)JS_GetPointerParameter(ctx, 0), key, buf, &required_size);
    JS_FreeString(ctx, key);
    if (ret) {
        free(buf);
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_STRING_FREE(ctx, buf);
#else
    MDB_txn *txn = NULL;
    MDB_dbi dbi;
    MDB_env *env = (MDB_env *)JS_GetPointerParameter(ctx, 0);
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);

    MDB_val db_key, data;

    db_key.mv_size = strlen(key);
    db_key.mv_data = (void *)key;

    data.mv_size = 0;
    data.mv_data = NULL;

    mdb_open(txn, NULL, 0, &dbi);

    int err = mdb_get(txn, dbi, &db_key, &data);
    if (err) {
        mdb_txn_abort(txn);
        mdb_close(env, dbi);
    }
#ifdef WITH_DUKTAPE
    duk_push_lstring(ctx, (char *)data.mv_data, data.mv_size);
    mdb_txn_abort(txn);
    mdb_close(env, dbi);
    return 1;
#else
    JSValue val = JS_NewStringLen(ctx, (char *)data.mv_data, data.mv_size);
    mdb_txn_abort(txn);
    mdb_close(env, dbi);
    return val;
#endif
#endif
}

JS_C_FUNCTION(js_nvs_erase_key) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
#ifdef ESP32
    esp_err_t ret = nvs_erase_key((nvs_handle)JS_GetPointerParameter(ctx, 0), key);
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
#else
    MDB_txn *txn = NULL;
    MDB_dbi dbi;
    MDB_env *env = (MDB_env *)JS_GetPointerParameter(ctx, 0);
    mdb_txn_begin(env, NULL, 0, &txn);

    MDB_val db_key;

    db_key.mv_size = strlen(key);
    db_key.mv_data = (void *)key;

    mdb_open(txn, NULL, 0, &dbi);

    int err = mdb_del(txn, dbi, &db_key, NULL);

    if (err)
        mdb_txn_abort(txn);
    else
        mdb_txn_commit(txn);
    mdb_close(env, dbi);

    JS_RETURN_NUMBER(ctx, err);
#endif
}

JS_C_FUNCTION(js_nvs_erase_all) {
    JS_ParameterPointer(ctx, 0);
#ifdef ESP32
    esp_err_t ret = nvs_erase_all((nvs_handle)JS_GetPointerParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, ret);
#else
    MDB_txn *txn = NULL;
    MDB_dbi dbi;
    MDB_env *env = (MDB_env *)JS_GetPointerParameter(ctx, 0);
    mdb_txn_begin(env, NULL, 0, &txn);
    mdb_open(txn, NULL, 0, &dbi);
    int err = mdb_drop(txn, dbi, 0);
    if (err)
        mdb_txn_abort(txn);
    else
        mdb_txn_commit(txn);
    mdb_close(env, dbi);
    JS_RETURN_NUMBER(ctx, err);
#endif
}

void register_nvs_functions(void *main_loop, void *js_ctx) {
    JS_CONTEXT ctx = (JS_CONTEXT )js_ctx;

    register_object(ctx, "nvs",
        "init", js_nvs_flash_init,
        "deinit", js_nvs_flash_deinit,
        "erase", js_nvs_flash_erase,
        "open", js_nvs_open,
#ifdef ESP32
        "erasePartition", js_nvs_flash_erase_partition,
        "initPartion", js_nvs_flash_init_partition,
        "openFromPartition", js_nvs_open_from_partition,
        "setByte", js_nvs_set_u8,
        "setInt16", js_nvs_set_i16,
        "setUint16", js_nvs_set_u16,
        "setInt32", js_nvs_set_i32,
        "setUint32", js_nvs_set_u32,
        "setInt64", js_nvs_set_i64,
        "setUint64", js_nvs_set_u64,
        "getByte", js_nvs_get_u8,
        "getInt16", js_nvs_get_i16,
        "getUint16", js_nvs_get_u16,
        "getInt32", js_nvs_get_i32,
        "getUint32", js_nvs_get_u32,
        "getInt64", js_nvs_get_i64,
        "getUint64", js_nvs_get_u64,
#endif
        "setStr", js_nvs_set_str,
        "getStr", js_nvs_get_str,
        "eraseKey", js_nvs_erase_key,
        "eraseAll", js_nvs_erase_all,
        "commit", js_nvs_commit,
        "close", js_nvs_close,
        NULL, NULL
    );
    JS_EvalSimple(ctx, "nvs.constants = { NVS_READWRITE: 1, NVS_READONLY: 0, ESP_ERR_NVS_NO_FREE_PAGES: 0x110d, ESP_ERR_NVS_KEYS_NOT_INITIALIZED: 0x1116, ESP_ERR_NVS_NEW_VERSION_FOUND: 0x1110, ESP_ERR_NVS_NOT_FOUND: 0x1102 }");
}
