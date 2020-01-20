#include "builtins.h"
#include "builtins_nvs.h"

#include <nvs_flash.h>

#define STR_HELPER(x)   #x
#define TO_STR(x)       STR_HELPER(x)

JS_C_FUNCTION(js_nvs_flash_init) {
    esp_err_t ret = nvs_flash_init();
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_flash_deinit) {
    esp_err_t ret = nvs_flash_deinit();
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_flash_erase) {
    esp_err_t ret = nvs_flash_deinit();
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_flash_erase_partition) {
    JS_ParameterString(ctx, 0);
    const char *partition = JS_GetStringParameter(ctx, 0);
    esp_err_t ret = nvs_flash_erase_partition(partition);
    JS_FreeString(ctx, partition);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_flash_init_partition) {
    JS_ParameterString(ctx, 0);
    const char *partition = JS_GetStringParameter(ctx, 0);
    esp_err_t ret = nvs_flash_init_partition(partition);
    JS_FreeString(ctx, partition);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_open) {
    JS_ParameterString(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    nvs_handle out_handle = NULL;
    const char *str = JS_GetStringParameter(ctx, 0);
    esp_err_t ret = nvs_open(str, JS_GetIntParameter(ctx, 1), &out_handle);
    JS_FreeString(ctx, str);
    if (!ret)
        JS_RETURN_POINTER(ctx, (void *)out_handle);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_nvs_open_from_partition) {
    JS_ParameterString(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);
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

JS_C_FUNCTION(js_nvs_close) {
    JS_ParameterPointer(ctx, 0);
    nvs_close((nvs_handle)JS_GetPointerParameter(ctx, 0));
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_nvs_commit) {
    JS_ParameterPointer(ctx, 0);
    esp_err_t ret = nvs_commit((nvs_handle)JS_GetPointerParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, ret);
}

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

JS_C_FUNCTION(js_nvs_set_str) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterString(ctx, 2);
    const char *key = JS_GetStringParameter(ctx, 1);
    const char *str = JS_GetStringParameter(ctx, 2);
    esp_err_t ret = nvs_set_str((nvs_handle)JS_GetPointerParameter(ctx, 0), key, str);
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
    char *buf = (char *)malloc(required_size);
    esp_err_t ret = nvs_get_str((nvs_handle)JS_GetPointerParameter(ctx, 0), key, buf, &required_size);
    JS_FreeString(ctx, key);
    if (ret) {
        free(buf);
        JS_RETURN_UNDEFINED(ctx);
    }
    JS_RETURN_STRING_FREE(ctx, buf);
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

JS_C_FUNCTION(js_nvs_erase_key) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    const char *key = JS_GetStringParameter(ctx, 1);
    esp_err_t ret = nvs_erase_key((nvs_handle)JS_GetPointerParameter(ctx, 0), key);
    JS_FreeString(ctx, key);
    JS_RETURN_NUMBER(ctx, ret);
}

JS_C_FUNCTION(js_nvs_erase_all) {
    JS_ParameterPointer(ctx, 0);
    esp_err_t ret = nvs_erase_all((nvs_handle)JS_GetPointerParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, ret);
}

void register_nvs_functions(void *main_loop, void *js_ctx) {
    JS_CONTEXT ctx = (JS_CONTEXT )js_ctx;

    register_object(ctx, "nvs",
        "init", js_nvs_flash_init,
        "deinit", js_nvs_flash_deinit,
        "erase", js_nvs_flash_erase,
        "erasePartition", js_nvs_flash_erase_partition,
        "initPartion", js_nvs_flash_init_partition,
        "open", js_nvs_open,
        "openFromPartition", js_nvs_open_from_partition,
        "setByte", js_nvs_set_u8,
        "setInt16", js_nvs_set_i16,
        "setUint16", js_nvs_set_u16,
        "setInt32", js_nvs_set_i32,
        "setUint32", js_nvs_set_u32,
        "setInt64", js_nvs_set_i64,
        "setUint64", js_nvs_set_u64,
        "setStr", js_nvs_set_str,
        "getByte", js_nvs_get_u8,
        "getInt16", js_nvs_get_i16,
        "getUint16", js_nvs_get_u16,
        "getInt32", js_nvs_get_i32,
        "getUint32", js_nvs_get_u32,
        "getInt64", js_nvs_get_i64,
        "getUint64", js_nvs_get_u64,
        "getStr", js_nvs_get_str,
        "eraseKey", js_nvs_erase_key,
        "eraseAll", js_nvs_erase_all,
        "commit", js_nvs_commit,
        "close", js_nvs_close,
        NULL, NULL
    );
    JS_EvalSimple(ctx, "nvs.constants = { NVS_READWRITE: 1, NVS_READONLY: 0,ESP_ERR_NVS_NO_FREE_PAGES: " TO_STR(ESP_ERR_NVS_NO_FREE_PAGES) ", ESP_ERR_NVS_KEYS_NOT_INITIALIZED: " TO_STR(ESP_ERR_NVS_KEYS_NOT_INITIALIZED) ",  ESP_ERR_NVS_NEW_VERSION_FOUND: " TO_STR(ESP_ERR_NVS_NEW_VERSION_FOUND) ", ESP_ERR_NVS_NOT_FOUND: " TO_STR(ESP_ERR_NVS_NOT_FOUND) " }");
}
