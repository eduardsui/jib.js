#ifndef JS_ENGINE_H
#define JS_ENGINE_H

#define WITH_DUKTAPE

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

    #define JS_GetNumberParameter(ctx, index)                   duk_get_number((ctx), (index))
    #define JS_GetIntParameter(ctx, index)                      (int)duk_get_number((ctx), (index))
    #define JS_GetBooleanParameter(ctx, index)                  duk_get_boolean((ctx), (index))
    #define JS_GetStringParameter(ctx, index)                   duk_get_string((ctx), (index))
    #define JS_GetStringParameterLen(ctx, index, len)           duk_get_lstring((ctx), (index), (len))
    #define JS_GetAsStringParameter(ctx, index)                 duk_safe_to_string((ctx), (index))
    #define JS_GetAsStringParameterLen(ctx, index, len)         duk_safe_to_lstring((ctx), (index), (len))
    #define JS_GetPointerParameter(ctx, index)                  duk_get_pointer_default((ctx), (index), NULL)
    #define JS_GetBufferParameter(ctx, index, sz)               duk_require_buffer_data((ctx), (index), (sz))
    
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

    #define JS_Error(ctx, text)                                 duk_error(ctx, DUK_ERR_TYPE_ERROR, text)


    #define js_c_function                                       duk_c_function
    #define JS_C_FUNCTION(name)                                 static duk_ret_t name(JS_CONTEXT ctx)

    #define JS_RETURN_NUMBER(ctx, val)                          { duk_push_number((ctx), (val)); return 1; }
    #define JS_RETURN_BOOLEAN(ctx, val)                         { duk_push_boolean((ctx), (val)); return 1; }
    #define JS_RETURN_STRING(ctx, val)                          { duk_push_string((ctx), (val)); return 1; }
    #define JS_RETURN_POINTER(ctx, val)                         { duk_push_pointer((ctx), (val)); return 1; }
    #define JS_RETURN_NULL(ctx)                                 { duk_push_null((ctx)); return 1; }
    #define JS_RETURN_UNDEFINED(ctx)                            { duk_push_undefined((ctx)); return 1; }
    #define JS_RETURN_BUFFER(ctx, val, len)                     { void *p = duk_push_fixed_buffer(ctx, len); memcpy(p, (val), (len)); duk_push_buffer_object((ctx), -1, 0, (len), DUK_BUFOBJ_NODEJS_BUFFER); return 1; }
    #define JS_RETURN_BUFFER_FREE_VAL(ctx, val, len)            { void *p = duk_push_fixed_buffer(ctx, len); memcpy(p, (val), (len)); free((val)); duk_push_buffer_object((ctx), -1, 0, (len), DUK_BUFOBJ_NODEJS_BUFFER); return 1; }
    #define JS_RETURN_NOTHING(ctx)                              return 0

    static inline js_object_type JS_NewObject(JS_CONTEXT ctx, const char *class_name) {
        duk_get_global_string(ctx, class_name);
        duk_new(ctx, 0);
        return duk_get_top(ctx) - 1;
    }

    static inline js_object_type JS_NewPlainObject(JS_CONTEXT ctx) {
        duk_push_object(ctx);
        return duk_get_top(ctx) - 1;
    }
#endif

#endif
