#include "builtins.h"
#include "builtins_gpio.h"
#include "doops.h"

#ifdef NO_TLS
    #include "misc/curve25519.c"
#else
    static const unsigned char kCurve25519BasePoint[ 32 ] = { 9 };
    void curve25519(uint8_t *mypublic, const uint8_t *secret, const uint8_t *basepoint);
#endif

#include "misc/chacha20.c"

int base32_decode(const uint8_t *encoded, uint8_t *result, int bufSize) {
    int buffer = 0;
    int bitsLeft = 0;
    int count = 0;
    for (const uint8_t *ptr = encoded; count < bufSize && *ptr; ++ptr) {
        uint8_t ch = *ptr;
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-')
            continue;
        buffer <<= 5;

        if (ch == '0')
            ch = 'O';
        else
        if (ch == '1')
            ch = 'L';
        else
        if (ch == '8')
            ch = 'B';

        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
            ch = (ch & 0x1F) - 1;
        else
        if (ch >= '2' && ch <= '7')
            ch -= '2' - 26;
        else
            return -1;

        buffer |= ch;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            result[count++] = buffer >> (bitsLeft - 8);
            bitsLeft -= 8;
        }
    }
    if (count < bufSize)
        result[count] = '\000';
    return count;
}

int base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize) {
    if (length < 0 || length > (1 << 28))
        return -1;

    int count = 0;
    if (length > 0) {
        int buffer = data[0];
        int next = 1;
        int bitsLeft = 8;
        while (count < bufSize && (bitsLeft > 0 || next < length)) {
            if (bitsLeft < 5) {
                if (next < length) {
                    buffer <<= 8;
                    buffer |= data[next++] & 0xFF;
                    bitsLeft += 8;
                } else {
                    int pad = 5 - bitsLeft;
                    buffer <<= pad;
                    bitsLeft += pad;
                }
            }
            int index = 0x1F & (buffer >> (bitsLeft - 5));
            bitsLeft -= 5;
            result[count++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[index];
        }
    }
    if (count < bufSize)
        result[count] = '\000';
    return count;
}

JS_C_FUNCTION(js_x25519) {
    if (JS_ParameterCount(ctx) < 2)
        JS_RETURN_NOTHING(ctx);

    js_size_t mypublic_sz;
    void *mypublic = JS_GetBufferParameter(ctx, 0, &mypublic_sz);

    js_size_t secret_sz;
    void *secret = JS_GetBufferParameter(ctx, 1, &secret_sz);

    if ((!mypublic) || (mypublic_sz != 32) || (!secret) || (secret_sz != 32)) {
        JS_Error(ctx, "public/secret size must be 32 bytes long");
        JS_RETURN_NOTHING(ctx);
    }

    if (JS_ParameterCount(ctx) > 2) {
        js_size_t basepoint_sz;
        void *basepoint = JS_GetBufferParameter(ctx, 2, &basepoint_sz);

        if ((!basepoint) || (basepoint_sz != 32)) {
            JS_Error(ctx, "basepoint size must be 32 bytes long");
            JS_RETURN_NOTHING(ctx);
        }

        curve25519((uint8_t *)mypublic, (const uint8_t *)secret, (const uint8_t *)basepoint);
    } else {
        curve25519((uint8_t *)mypublic, (const uint8_t *)secret, kCurve25519BasePoint);
    }
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_chacha20) {
    if (JS_ParameterCount(ctx) < 2)
        JS_RETURN_NOTHING(ctx);

    js_size_t buffer_sz;
    void *buffer = JS_GetBufferParameter(ctx, 0, &buffer_sz);
    if ((!buffer) || (!buffer_sz))
        JS_RETURN_NOTHING(ctx);

    js_size_t secret_sz;
    void *secret = JS_GetBufferParameter(ctx, 1, &secret_sz);

    if ((!secret) || (secret_sz < 16)) {
        JS_Error(ctx, "key size must by at least 16 bytes long");
        JS_RETURN_NOTHING(ctx);
    }


    ECRYPT_ctx x;
    ECRYPT_keysetup(&x, (const u8 *)secret, secret_sz * 8, 0);

    if (JS_ParameterCount(ctx) > 2) {
        js_size_t iv_sz;
        void *iv = JS_GetBufferParameter(ctx, 2, &iv_sz);

        if ((!iv) || (iv_sz < 8)) {
            JS_Error(ctx, "iv size must be at least 8 long");
            JS_RETURN_NOTHING(ctx);
        }
        ECRYPT_ivsetup(&x, (const u8 *)iv);
    } else {
        static const u8 default_iv[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        ECRYPT_ivsetup(&x, default_iv);
    }
#ifdef WITH_DUKTAPE
    unsigned char *buf = (unsigned char *)duk_push_fixed_buffer(ctx, buffer_sz);
    if (!buf)
        JS_RETURN_NOTHING(ctx);

    ECRYPT_encrypt_bytes(&x, (const u8 *)buffer, (u8 *)buf, buffer_sz);
    duk_push_buffer_object(ctx, -1, 0, buffer_sz, DUK_BUFOBJ_NODEJS_BUFFER);
    return 1;
#else
    unsigned char *buf = (unsigned char *)malloc(buffer_sz);
    if (!buf)
        JS_RETURN_NOTHING(ctx);

    ECRYPT_encrypt_bytes(&x, (const u8 *)buffer, buf, buffer_sz);
    JSValue val = JS_NewArrayBufferCopy(ctx, (const uint8_t *)buf, buffer_sz);
    free(buf);
    return val;
#endif
}

JS_C_FUNCTION(js_crc8) {
    uint8_t crc = 0xff;

    if (JS_ParameterCount(ctx) < 1)
        JS_RETURN_NUMBER(ctx, crc);

    js_size_t buffer_sz;
    unsigned char *buffer = (unsigned char *)JS_GetBufferParameter(ctx, 0, &buffer_sz);

    js_size_t i, j;
    for (i = 0; i < buffer_sz; i++) {
        crc ^= buffer[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    JS_RETURN_NUMBER(ctx, crc);
}

JS_C_FUNCTION(js_crc16) {
    if (JS_ParameterCount(ctx) < 1)
        JS_RETURN_NUMBER(ctx, 0);

    js_size_t buffer_sz;
    unsigned char *buffer = (unsigned char *)JS_GetBufferParameter(ctx, 0, &buffer_sz);

    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    while(buffer_sz > 0) {
        bit_flag = out >> 15;

        out <<= 1;
        out |= (*buffer >> bits_read) & 1;

        bits_read++;
        if(bits_read > 7) {
            bits_read = 0;
            buffer ++;
            buffer_sz --;
        }

        if(bit_flag)
            out ^= 0x8005;
    }

    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if (bit_flag)
            out ^= 0x8005;
    }

    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out)
            crc |= j;
    }

    JS_RETURN_NUMBER(ctx, crc);
}

JS_C_FUNCTION(js_b32_encode) {
    if (JS_ParameterCount(ctx) < 1)
        JS_RETURN_NOTHING(ctx);

    js_size_t buffer_sz;
    unsigned char *buffer = (unsigned char *)JS_GetBufferParameter(ctx, 0, &buffer_sz);

    char *out_buf = (char *)malloc(buffer_sz * 2);
    if (!out_buf) {
        JS_RETURN_NOTHING(ctx);
    }
    int len = base32_encode(buffer, buffer_sz, (unsigned char *)out_buf, buffer_sz * 2);
    if (len < 0) {
        free(out_buf);
        JS_RETURN_NOTHING(ctx);
    }
    JS_RETURN_STRING_FREE(ctx, out_buf);
}

JS_C_FUNCTION(js_b32_decode) {
    JS_ParameterString(ctx, 0);

    js_size_t len = 0;
    const char *str = JS_GetStringParameterLen(ctx, 0, &len);
#ifdef WITH_DUKTAPE
    unsigned char *buf = (unsigned char *)duk_push_fixed_buffer(ctx, len);
    if (!buf) {
        JS_RETURN_NOTHING(ctx);
    }
    int decoded_len = base32_decode((const unsigned char *)str, buf, len);
    if (decoded_len < 0) {
        duk_pop(ctx);
        JS_RETURN_NOTHING(ctx);
    }
    duk_push_buffer_object(ctx, -1, 0, decoded_len, DUK_BUFOBJ_UINT8ARRAY);
    return 1;
#else
    unsigned char *buf = (unsigned char *)malloc(len);
    if (!buf) {
        JS_RETURN_NOTHING(ctx);
    }
    int decoded_len = base32_decode((const unsigned char *)str, buf, len);
    if (decoded_len < 0) {
        free(buf);
        JS_RETURN_NOTHING(ctx);
    }
    js_object_type val = JS_NewArrayBufferCopy(ctx, (const uint8_t *)buf, decoded_len);
    free(buf);
    return val;
#endif
}

void register_crypto_functions(void *main_loop, void *js_ctx) {
    JS_CONTEXT ctx = (JS_CONTEXT )js_ctx;

    register_object(ctx, "_crypto_",
        "x25519", js_x25519,
        "chacha20", js_chacha20,
        "crc8", js_crc8,
        "crc16", js_crc16,
        "b32Encode", js_b32_encode,
        "b32Decode", js_b32_decode,
        NULL, NULL
    );
}
