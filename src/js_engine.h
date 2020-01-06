#ifndef JS_ENGINE_H
#define JS_ENGINE_H

#ifndef WITH_QUICKJS
    #define WITH_DUKTAPE
#endif

#ifdef WITH_DUKTAPE
    #include "duktape/duktape.h"

    #define JS_CONTEXT                                          duk_context *
    #define JS_CreateContext(on_error)                          duk_create_heap(NULL, NULL, NULL, NULL, (on_error))
    #define JS_DestroyContext(ctx)                              duk_destroy_heap((ctx))
    #define JS_EvalSimple(ctx, str)                             duk_eval_string_noresult(ctx, str)
    #define JS_Eval(ctx, str, path)                             {                                                               \
	                                                                duk_push_string(ctx, path);                                 \
	                                                                duk_compile_string_filename(ctx, DUK_COMPILE_EVAL | DUK_COMPILE_SHEBANG, str);   \
	                                                                duk_push_global_object(ctx);                                \
	                                                                duk_call_method(ctx, 0);                                    \
                                                                    /* duk_pop(ctx); */                                         \
                                                                }

    #define JS_Eval_File(ctx, filename)                         duk_run_file((ctx), filename)

    #define JS_ParameterNumber(ctx, index)                      duk_require_number((ctx), (index))
    #define JS_ParameterBoolean(ctx, index)                     duk_require_boolean((ctx), (index))
    #define JS_ParameterString(ctx, index)                      duk_require_string((ctx), (index))
    #define JS_ParameterFunction(ctx, index)                    duk_require_function((ctx), (index))
    #define JS_ParameterObject(ctx, index)                      duk_require_object((ctx), (index))
    #define JS_ParameterPointer(ctx, index)                     duk_require_pointer((ctx), (index))
    #define JS_ParameterCount(ctx)                              duk_get_top((ctx))
    
    #define JS_FreeString(ctx, str)

    #define JS_GetNumberParameter(ctx, index)                   duk_get_number((ctx), (index))
    #define JS_GetIntParameter(ctx, index)                      (int)duk_get_number((ctx), (index))
    #define JS_GetBooleanParameter(ctx, index)                  duk_get_boolean((ctx), (index))
    #define JS_GetStringParameter(ctx, index)                   duk_get_string((ctx), (index))
    #define JS_GetStringParameterLen(ctx, index, len)           duk_get_lstring((ctx), (index), (len))
    #define JS_GetAsStringParameter(ctx, index)                 duk_safe_to_string((ctx), (index))
    #define JS_GetAsStringParameterLen(ctx, index, len)         duk_safe_to_lstring((ctx), (index), (len))
    #define JS_GetPointerParameter(ctx, index)                  duk_get_pointer_default((ctx), (index), NULL)
    #define JS_GetBufferParameter(ctx, index, sz)               duk_require_buffer_data((ctx), (index), (sz))
    #define JS_GetObjectParameter(ctx, index)                   index
    
    #define JS_ObjectSetNumber(ctx, obj, mem, val)              { duk_push_number((ctx), (val)); duk_put_prop_string((ctx), (obj), (mem)); }
    #define JS_ObjectSetString(ctx, obj, mem, val)              { duk_push_string((ctx), (val)); duk_put_prop_string((ctx), (obj), (mem)); }
    #define JS_ObjectSetStringLen(ctx, obj, mem, val, len)      { duk_push_lstring((ctx), (val), (len)); duk_put_prop_string((ctx), (obj), (mem)); }
    #define JS_ObjectSetStringLenLen(ctx, obj, mem, lm, val, lv){ duk_push_lstring((ctx), (val), (lv)); duk_put_prop_lstring((ctx), (obj), (mem), (lm)); }
    #define JS_ObjectSetPointer(ctx, obj, mem, val)             { duk_push_pointer((ctx), (val)); duk_put_prop_string((ctx), (obj), (mem)); }
    #define JS_ObjectSetObject(ctx, obj, mem, obj_id)           duk_put_prop_string((ctx), (obj), (mem));
    #define JS_ObjectSetFunction(ctx, obj, mem, f, len)         { duk_push_c_function((ctx), (f), (len)); duk_put_prop_string((ctx), (obj), (mem)); }
    #define JS_ObjectSetBoolean(ctx, obj, mem, val)             { duk_push_boolean((ctx), (val)); duk_put_prop_string((ctx), (obj), (mem)); }

    #define JS_HIDDEN_SYMBOL                                    DUK_HIDDEN_SYMBOL
    #define JS_VARARGS                                          DUK_VARARGS

    #define js_object_type                                      duk_idx_t
    #define js_size_t                                           duk_size_t
    #define js_uint_t                                           duk_uint_t

    #define JS_Error(ctx, text)                                 duk_error(ctx, DUK_ERR_TYPE_ERROR, text)


    #define js_c_function                                       duk_c_function
    #define JS_C_FUNCTION(name)                                 static duk_ret_t name(JS_CONTEXT ctx)
    #define JS_C_FUNCTION_FORWARD(name, ...)                    static duk_ret_t name(JS_CONTEXT ctx, __VA_ARGS__)
    #define JS_C_FORWARD_PARAMETERS                             ctx

    #define JS_RETURN_NUMBER(ctx, val)                          { duk_push_number((ctx), (val)); return 1; }
    #define JS_RETURN_BOOLEAN(ctx, val)                         { duk_push_boolean((ctx), (val)); return 1; }
    #define JS_RETURN_STRING(ctx, val)                          { duk_push_string((ctx), (val)); return 1; }
    #define JS_RETURN_POINTER(ctx, val)                         { duk_push_pointer((ctx), (val)); return 1; }
    #define JS_RETURN_NULL(ctx)                                 { duk_push_null((ctx)); return 1; }
    #define JS_RETURN_UNDEFINED(ctx)                            { duk_push_undefined((ctx)); return 1; }
    #define JS_RETURN_BUFFER(ctx, val, len)                     { void *p = duk_push_fixed_buffer(ctx, len); memcpy(p, (val), (len)); duk_push_buffer_object((ctx), -1, 0, (len), DUK_BUFOBJ_NODEJS_BUFFER); return 1; }
    #define JS_RETURN_BUFFER_FREE_VAL(ctx, val, len)            { void *p = duk_push_fixed_buffer(ctx, len); memcpy(p, (val), (len)); free((val)); duk_push_buffer_object((ctx), -1, 0, (len), DUK_BUFOBJ_NODEJS_BUFFER); return 1; }
    #define JS_RETURN_NOTHING(ctx)                              return 0
    #define JS_RETURN_OBJECT(ctx, obj)                          return 1

    static inline js_object_type JS_NewObject(JS_CONTEXT ctx, const char *class_name) {
        duk_get_global_string(ctx, class_name);
        duk_new(ctx, 0);
        return duk_get_top(ctx) - 1;
    }

    static inline js_object_type JS_NewPlainObject(JS_CONTEXT ctx) {
        duk_push_object(ctx);
        return duk_get_top(ctx) - 1;
    }
#else
    #include "quickjs/quickjs.h"

    #define JS_CONTEXT                                          JSContext *
    #define JS_CreateContext(on_error)                          JS_NewContextRaw(JS_NewRuntime())
    #define JS_DestroyContext(ctx)                              { JSRuntime *rt = JS_GetRuntime(ctx); JS_FreeContext(ctx); JS_FreeRuntime(rt); }
    #define JS_EvalSimple(ctx, str)                             JS_FreeValue(ctx, (JS_Eval)(ctx, str, strlen(str), "builtin", JS_EVAL_TYPE_GLOBAL));
    #define JS_EvalSimplePath(ctx, str, path)                   JS_FreeValue(ctx, (JS_Eval)(ctx, str, strlen(str), path, JS_EVAL_TYPE_GLOBAL));
    #define JS_Eval(ctx, str, path)                             (JS_Eval)(ctx, str, strlen(str), path, JS_EVAL_TYPE_GLOBAL)

    #define JS_Eval_File(ctx, filename)                         duk_run_file((ctx), filename)

    #define JS_ParameterNumber(ctx, index)                      if (!JS_IsNumber(argv[(index)])) JS_ThrowTypeError(ctx, "Number expected in parameter %i", (int)index)
    #define JS_ParameterBoolean(ctx, index)                     if (!JS_IsBool(argv[(index)])) JS_ThrowTypeError(ctx, "Boolean value expected in parameter %i", (int)index)
    #define JS_ParameterString(ctx, index)                      if (!JS_IsString(argv[(index)])) JS_ThrowTypeError(ctx, "String expected in parameter %i", (int)index)
    #define JS_ParameterFunction(ctx, index)                    if (!JS_IsFunction((ctx), argv[(index)])) JS_ThrowTypeError(ctx, "Function expected in parameter %i", (int)index)
    #define JS_ParameterObject(ctx, index)                      if (!JS_IsObject(argv[(index)])) JS_ThrowTypeError(ctx, "Object expected in parameter %i", (int)index)
    #define JS_ParameterPointer(ctx, index)                     if (!JS_IsInteger(argv[(index)])) JS_ThrowTypeError(ctx, "Handle expected in parameter %i", (int)index)
    #define JS_ParameterCount(ctx)                              argc

    #define JS_FreeString(ctx, str)                             JS_FreeCString(ctx, str)

    #define JS_GetNumberParameter(ctx, index)                   _JS_GetNumberParameter((ctx), argv[(index)])
    #define JS_GetIntParameter(ctx, index)                      _JS_GetIntParameter((ctx), argv[(index)])
    #define JS_GetBooleanParameter(ctx, index)                  _JS_GetBooleanParameter((ctx), argv[(index)])
    #define JS_GetStringParameter(ctx, index)                   JS_ToCString((ctx), argv[(index)])
    #define JS_GetStringParameterLen(ctx, index, len)           JS_ToCStringLen((ctx), (len), argv[(index)])
    #define JS_GetAsStringParameter(ctx, index)                 JS_GetStringParameter(ctx, index)
    #define JS_GetAsStringParameterLen(ctx, index, len)         JS_GetStringParameterLen(ctx, index, len)
    #define JS_GetPointerParameter(ctx, index)                  _JS_GetPointerParameter((ctx), argv[(index)])
    #define JS_GetBufferParameter(ctx, index, sz)               JS_GetArrayBuffer((ctx), (sz), argv[(index)])
    #define JS_GetObjectParameter(ctx, index)                   argv[(index)]
    
    #define JS_ObjectSetNumber(ctx, obj, mem, val)              JS_SetPropertyStr((ctx), (obj), (mem), JS_NewInt32((ctx), (val)))
    #define JS_ObjectSetString(ctx, obj, mem, val)              JS_SetPropertyStr((ctx), (obj), (mem), JS_NewString((ctx), (val)))
    #define JS_ObjectSetStringLen(ctx, obj, mem, val, len)      JS_SetPropertyStr((ctx), (obj), (mem), JS_NewStringLen((ctx), (val), (len)))
    #define JS_ObjectSetStringLenLen(ctx, obj, mem, lm, val, lv) _JS_ObjectSetStringLenLen((ctx), (obj), (mem), (lm), (val), (lv))
#ifdef ESP32
    #define JS_ObjectSetPointer(ctx, obj, mem, val)             JS_SetPropertyStr((ctx), (obj), (mem), JS_NewUint32((ctx), (uint32_t)(val)))
#else
    #define JS_ObjectSetPointer(ctx, obj, mem, val)             JS_SetPropertyStr((ctx), (obj), (mem), JS_NewBigUint64((ctx), (uint64_t)(val)))
#endif
    #define JS_ObjectSetObject(ctx, obj, mem, obj_id)           JS_SetPropertyStr((ctx), (obj), (mem), (obj_id))
    #define JS_ObjectSetFunction(ctx, obj, mem, f, len)         JS_SetPropertyStr((ctx), (obj), (mem), JS_NewCFunction((ctx), (f), (mem), (len)))
    #define JS_ObjectSetBoolean(ctx, obj, mem, val)             JS_SetPropertyStr((ctx), (obj), (mem), (val) ? JS_TRUE : JS_FALSE)
    #define JS_ObjectSetNull(ctx, obj, mem)                     JS_SetPropertyStr((ctx), (obj), (mem), JS_NULL)
    #define JS_ObjectSetUndefined(ctx, obj, mem)                JS_SetPropertyStr((ctx), (obj), (mem), JS_UNDEFINED)

    #define JS_HIDDEN_SYMBOL
    #define JS_VARARGS                                          1

    #define js_object_type                                      JSValue
    #define js_size_t                                           size_t
    #define js_uint_t                                           unsigned int

    #define JS_Error(ctx, text)                                 JS_ThrowTypeError(ctx, "%s", text)

    #define js_c_function                                       JSCFunction *
    #define JS_C_FUNCTION(name)                                 static JSValue name(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
    #define JS_C_FUNCTION_FORWARD(name, ...)                    static JSValue name(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, __VA_ARGS__)
    #define JS_C_FORWARD_PARAMETERS                             ctx, this_val, argc, argv

    #define JS_RETURN_NUMBER(ctx, val)                          return JS_NewInt32((ctx), (val))
    #define JS_RETURN_BOOLEAN(ctx, val)                         return ((val) ? JS_TRUE : JS_FALSE)
    #define JS_RETURN_STRING(ctx, val)                          return JS_NewString((ctx), (val))
#ifdef ESP32
    #define JS_RETURN_POINTER(ctx, val)                         return JS_NewUint32((ctx), (uintptr_t)(val))
#else
    #define JS_RETURN_POINTER(ctx, val)                         return JS_NewBigUint64((ctx), (uintptr_t)(val))
#endif
    #define JS_RETURN_NULL(ctx)                                 return JS_NULL
    #define JS_RETURN_UNDEFINED(ctx)                            return JS_UNDEFINED
    #define JS_RETURN_BUFFER(ctx, val, len)                     return JS_NewArrayBufferCopy((ctx), (const uint8_t *)(val), (size_t)(len))
    #define JS_RETURN_BUFFER_FREE_VAL(ctx, val, len)            { JSValue ret = JS_NewArrayBufferCopy((ctx), (const uint8_t *)(val), (size_t)(len)); free((val)); return (ret); }
    #define JS_RETURN_NOTHING(ctx)                              return JS_UNDEFINED
    #define JS_RETURN_OBJECT(ctx, obj)                          return (obj)

    #define JS_NewObject(ctx, class_name)                       _JS_NewObject((ctx), (class_name))
    #define JS_NewPlainObject(ctx)                              (JS_NewObject)((ctx))

    static inline int _JS_GetIntParameter(JS_CONTEXT ctx, JSValueConst v) {
        uint32_t val = 0;
        if (JS_ToInt32(ctx, &val, v))
            JS_ThrowTypeError(ctx, "Number expected");
        return val;
    }

    static inline double _JS_GetNumberParameter(JS_CONTEXT ctx, JSValueConst v) {
        double val = 0;
        if (JS_ToFloat64(ctx, &val, v))
            JS_ThrowTypeError(ctx, "Number expected");
        return val;
    }

    static inline int _JS_GetBooleanParameter(JS_CONTEXT ctx, JSValueConst v) {
        int val = JS_ToBool(ctx, v);
        if (val == -1)
            JS_ThrowTypeError(ctx, "Boolean expected");
        return val;
    }

    static inline void *_JS_GetPointerParameter(JS_CONTEXT ctx, JSValueConst v) {
#ifdef ESP32
        uint32 val = 0;
        if (JS_ToUint32(ctx, &val, v))
            JS_ThrowTypeError(ctx, "Handle expected");
#else
        int64_t val = 0;
        if (JS_ToBigInt64(ctx, &val, v))
            JS_ThrowTypeError(ctx, "Handle expected");
#endif
        return (void *)(uintptr_t)val;
    }

    static inline void _JS_ObjectSetStringLenLen(JS_CONTEXT ctx, JSValue obj, const char *mem, int len_mem, const char *val, int len_val) {
        char mem_buf[0x100];
        if (len_mem >= sizeof(mem_buf))
            len_mem = sizeof(mem_buf) - 1;

        if (len_mem < 0)
            return;

        memcpy(mem_buf, mem, len_mem);
        JS_SetPropertyStr(ctx, obj, mem_buf, JS_NewStringLen(ctx, val, len_val));
    }

    static inline JSValue _JS_NewObject(JS_CONTEXT ctx, const char *class_name) {
        JSValue obj = (JS_NewObject)(ctx);
        JS_SetPrototype(ctx, obj, JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), class_name));
        return obj;
    }
#endif

#endif