#include "builtins.h"
#include "builtins_io.h"
#include "doops.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#ifndef NO_TLS
    #include "misc/tlse.h"
#endif

static unsigned char io_set = 0;

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

    #define SOL_TCP SOL_SOCKET

    const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt) {
        if (af == AF_INET) {
            struct sockaddr_in in;
            memset(&in, 0, sizeof(in));
            in.sin_family = AF_INET;
            memcpy(&in.sin_addr, src, sizeof(struct in_addr));
            getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
            return dst;
        } else if (af == AF_INET6) {
            struct sockaddr_in6 in;
            memset(&in, 0, sizeof(in));
            in.sin6_family = AF_INET6;
            memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
            getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
            return dst;
        }
        return NULL;
    }

    int inet_pton(int af, const char *src, void *dst) {
        struct sockaddr_storage ss;
        int  size = sizeof(ss);
        char src_copy[INET6_ADDRSTRLEN + 1];

        ZeroMemory(&ss, sizeof(ss));
        strncpy(src_copy, src, INET6_ADDRSTRLEN + 1);
        src_copy[INET6_ADDRSTRLEN] = 0;

        if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
            switch (af) {
                case AF_INET:
                    *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                    return 1;

                case AF_INET6:
                    *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                    return 1;
            }
        }
        return 0;
    }
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#ifndef ESP32
    #include <netinet/tcp.h>
#endif
#ifndef SOL_TCP
    #define SOL_TCP IPPROTO_TCP
#endif
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <fcntl.h>
#endif

#include "doops.h"

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen, int *port) {
    switch (sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
            if (port)
                *port = ntohs(((struct sockaddr_in *)sa)->sin_port);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
            if (port)
                *port = ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }
    return s;
}

int socket_blocking(int fd, int blocking) {
   if (fd < 0)
       return 0;

#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? 1 : 0;
#else
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1)
       return 0;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? 1 : 0;
#endif
}

JS_C_FUNCTION(js_socket) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterNumber(ctx, 2);
    int sock = (int)socket(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1), JS_GetIntParameter(ctx, 2));
    JS_RETURN_NUMBER(ctx, sock);
}

JS_C_FUNCTION(js_connect) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    int err = -1;

    struct addrinfo hints;
    struct addrinfo *res, *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[12];
    snprintf(port_str, sizeof(port_str), "%u", JS_GetIntParameter(ctx, 2));

    const char *hostname = JS_GetStringParameter(ctx, 1);
    if (getaddrinfo(hostname, port_str, &hints, &result) != 0)
        JS_RETURN_NUMBER(ctx, -1);

    for (res = result; res != NULL; res = res->ai_next) {
        err = connect(JS_GetIntParameter(ctx, 0), res->ai_addr, res->ai_addrlen);
        if (!err)
            break;
    }

    if (result)
        freeaddrinfo(result);
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_bind) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    int err = -1;

    int ipv6 = 0;
    if (JS_ParameterCount(ctx) > 3) {
        JS_ParameterBoolean(ctx, 3);
        ipv6 = JS_GetBooleanParameter(ctx, 3);
    }

    int sock = JS_GetIntParameter(ctx, 0);
    const char *iface = JS_GetStringParameter(ctx, 1);
    int port = JS_GetIntParameter(ctx, 2);

    if (ipv6) {
        struct sockaddr_in6 sin;
        memset(&sin, 0, sizeof(sin));

        if ((iface) && (iface[0]))
            inet_pton(AF_INET6, iface, (void *)&sin.sin6_addr.s6_addr);
        else
            sin.sin6_addr = in6addr_any;

        sin.sin6_family = AF_INET6;
        sin.sin6_port   = htons(port);

        err = bind((int)sock, (struct sockaddr *)&sin, sizeof(sin));
    } else {
        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));

        if ((iface) && (iface[0]))
            sin.sin_addr.s_addr = inet_addr(iface);
        else
            sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_family = AF_INET;
        sin.sin_port   = htons(port);

        err = bind((int)sock, (struct sockaddr *)&sin, sizeof(sin));
    }

    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_closesocket) {
    JS_ParameterNumber(ctx, 0);
#ifdef _WIN32
    int err = closesocket(JS_GetIntParameter(ctx, 0));
#else
    int err = close(JS_GetIntParameter(ctx, 0));
#endif
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_shutdown) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    int err = shutdown(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_listen) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    int err = listen(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_accept) {
    JS_ParameterNumber(ctx, 0);
    int sock = accept(JS_GetIntParameter(ctx, 0), NULL, NULL);
    JS_RETURN_NUMBER(ctx, sock);
}

JS_C_FUNCTION(js_setreuseaddr) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterBoolean(ctx, 1);

#ifdef _WIN32
    DWORD flags = JS_GetBooleanParameter(ctx, 1);
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_SOCKET, SO_REUSEADDR, (const char *)&flags, sizeof(flags));
#else
    int flags = JS_GetBooleanParameter(ctx, 1);
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
#endif
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_setnodelay) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterBoolean(ctx, 1);

    int flags = JS_GetBooleanParameter(ctx, 1);
#ifdef _WIN32
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_TCP, TCP_NODELAY, (const char *)&flags, sizeof(flags));
#else
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
#endif
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_setblocking) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterBoolean(ctx, 1);

    int is_set = socket_blocking(JS_GetIntParameter(ctx, 0), JS_GetIntParameter(ctx, 1));
    JS_RETURN_BOOLEAN(ctx, is_set);
}

JS_C_FUNCTION(js_settimeout) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);

#ifdef _WIN32
    DWORD timeout = JS_GetIntParameter(ctx, 0);
    setsockopt(JS_GetIntParameter(ctx, 0), SOL_TCP, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_TCP, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
#else
    int timeout_ms = JS_GetIntParameter(ctx, 0);
    struct timeval timeout;      
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    setsockopt(JS_GetIntParameter(ctx, 0), SOL_TCP, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_TCP, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
#endif
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_setkeepalive) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterBoolean(ctx, 1);

    int flags = JS_GetBooleanParameter(ctx, 1);
#ifdef _WIN32
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_SOCKET, SO_KEEPALIVE, (const char *)&flags, sizeof(flags));
#else
    int err = setsockopt(JS_GetIntParameter(ctx, 0), SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
#endif
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_geterror) {
    int error = 0;
    socklen_t len = sizeof (error);
#ifdef _WIN32
    int err = getsockopt(JS_GetIntParameter(ctx, 0), SOL_SOCKET, SO_ERROR, (char *)&error, &len);
#else
    int err = getsockopt(JS_GetIntParameter(ctx, 0), SOL_SOCKET, SO_ERROR, (void *)&error, &len);
#endif
    if (err)
        JS_RETURN_NUMBER(ctx, err);

    JS_RETURN_NUMBER(ctx, error);
}

JS_C_FUNCTION(js_socketerrno) {
#ifdef _WIN32
    JS_RETURN_NUMBER(ctx, WSAGetLastError());
#else
    JS_RETURN_NUMBER(ctx, errno);
#endif
}

JS_C_FUNCTION(js_socketstrerror) {
    JS_ParameterNumber(ctx, 0);
#ifdef _WIN32
    char msgbuf[0x100];
    msgbuf[0] = 0;

    FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, JS_GetIntParameter(ctx, 0), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msgbuf, sizeof(msgbuf), NULL);

    if (msgbuf[0]) {
        JS_RETURN_STRING(ctx, msgbuf);
    } else {
        JS_RETURN_STRING(ctx, "unknown error");
    }
#else
    JS_RETURN_STRING(ctx, strerror(JS_GetIntParameter(ctx, 0)));
#endif
}

JS_C_FUNCTION(js_recv) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 2);

    js_size_t sz = 0;
    void *buf = JS_GetBufferParameter(ctx, 1, &sz);
    ssize_t nbytes = (ssize_t)JS_GetNumberParameter(ctx, 2);
    if (nbytes < 0)
        nbytes = 0;
    ssize_t offset = 0;

    if (JS_ParameterCount(ctx) > 3) {
        JS_ParameterNumber(ctx, 3);
        offset = (ssize_t)JS_GetNumberParameter(ctx, 3);
        if (offset < 0)
            offset = 0;
    }

    if (offset >= sz)
        JS_RETURN_NUMBER(ctx, 0);

    if (sz < (offset + nbytes))
        nbytes = sz - offset;
    
    ssize_t bytes_read = recv(JS_GetIntParameter(ctx, 0), (unsigned char *)buf + offset, nbytes, 0);
    JS_RETURN_NUMBER(ctx, bytes_read);
}

JS_C_FUNCTION(js_recvfrom) {
    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 2);

    js_size_t sz = 0;
    void *buf = JS_GetBufferParameter(ctx, 1, &sz);
    size_t nbytes = (size_t)JS_GetNumberParameter(ctx, 2);
    ssize_t offset = 0;

    if (JS_ParameterCount(ctx) > 3) {
        JS_ParameterNumber(ctx, 3);
        offset = (ssize_t)JS_GetNumberParameter(ctx, 3);
        if (offset < 0)
            offset = 0;
        nbytes += offset;
    }

    if (sz < nbytes)
        nbytes = sz;

    if (offset >= sz)
        JS_RETURN_NUMBER(ctx, 0);
    
    struct sockaddr src_addr;
    socklen_t addrlen = sizeof(struct sockaddr);

    ssize_t bytes_read = recvfrom(JS_GetIntParameter(ctx, 0), (unsigned char *)buf + offset, nbytes - offset, 0, &src_addr, &addrlen);

    if ((bytes_read >= 0) && (JS_ParameterCount(ctx) > 4)) {
        JS_ParameterObject(ctx, 4);
        char ip_buf[0x100];
        int port = 0;
        char *str = get_ip_str(&src_addr, ip_buf, sizeof(ip_buf), &port);
        js_object_type obj_id = JS_GetObjectParameter(ctx, 4);
        JS_ObjectSetString(ctx, obj_id, "addr", str ? str : "");
        JS_ObjectSetNumber(ctx, obj_id, "port", port);
    }

    JS_RETURN_NUMBER(ctx, bytes_read);
}

JS_C_FUNCTION(js_send) {
    void *buf;
    js_size_t sz;
    JS_ParameterNumber(ctx, 0);
    buf = JS_GetBufferParameter(ctx, 1, &sz);

    ssize_t offset = 0;
    ssize_t nbytes = sz;
    int n = JS_ParameterCount(ctx);

    if (n > 2) {
        JS_ParameterNumber(ctx, 2);
        nbytes = (ssize_t)JS_GetNumberParameter(ctx, 2);
        if (nbytes < 0)
            JS_RETURN_NUMBER(ctx, 0);
    }
    if (n > 3) {
        JS_ParameterNumber(ctx, 3);
        offset = (ssize_t)JS_GetNumberParameter(ctx, 3);
        if (offset < 0)
            offset = 0;
    }
    if (offset >= sz)
        JS_RETURN_NUMBER(ctx, 0);

    if (offset + nbytes >= sz)
        nbytes = sz - offset;

    ssize_t bytes_written = send(JS_GetIntParameter(ctx, 0), (unsigned char *)buf + offset, nbytes, 0);
    JS_RETURN_NUMBER(ctx, bytes_written);
}

JS_C_FUNCTION(js_sendto) {
    void *buf;
    js_size_t sz;

    JS_ParameterNumber(ctx, 0);
    JS_ParameterString(ctx, 1);
    JS_ParameterNumber(ctx, 2);

    buf = JS_GetBufferParameter(ctx, 3, &sz);

    ssize_t offset = 0;
    ssize_t nbytes = sz;
    int n = JS_ParameterCount(ctx);

    if (n > 4) {
        JS_ParameterNumber(ctx, 4);
        nbytes = (ssize_t)JS_GetNumberParameter(ctx, 4);
        if (nbytes < 0)
            JS_RETURN_NUMBER(ctx, 0);
    }
    if (n > 5) {
        JS_ParameterNumber(ctx, 5);
        offset = (ssize_t)JS_GetNumberParameter(ctx, 5);
        if (offset < 0)
            offset = 0;
    }
    if (offset >= sz)
        JS_RETURN_NUMBER(ctx, 0);

    if (offset + nbytes >= sz)
        nbytes = sz - offset;

    char port_str[12];
    snprintf(port_str, sizeof(port_str), "%u", JS_GetIntParameter(ctx, 2));

    struct addrinfo hints;
    struct addrinfo *res, *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    const char *hostname = JS_GetStringParameter(ctx, 1);
    if (getaddrinfo(hostname, port_str, &hints, &result) != 0)
        JS_RETURN_NUMBER(ctx, -1);

    int err = -1;
    for (res = result; res != NULL; res = res->ai_next) {
        err = sendto(JS_GetIntParameter(ctx, 0), (unsigned char *)buf + offset, nbytes, 0, res->ai_addr, res->ai_addrlen);
        if (!err)
            break;
    }

    if (result)
        freeaddrinfo(result);

    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_socketinfo) {
    JS_ParameterNumber(ctx, 0);

    struct sockaddr_storage addr;
    char        ipstr[INET6_ADDRSTRLEN];
    int         port     = 0;
    static char *unknown = "unknown";
#ifdef _WIN32

    int peerlen = sizeof(addr);
#else
    socklen_t peerlen = sizeof(addr);
#endif

    js_object_type obj = JS_NewPlainObject(ctx);

    const char *ip = 0;
    int fd = JS_GetIntParameter(ctx, 0);
    if (!getpeername(fd, (struct sockaddr *)&addr, &peerlen)) {
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&addr;
            port = ntohs(s->sin_port);
            ip   = inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
        } else
        if (addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
            port = ntohs(s->sin6_port);
            ip   = inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
        }
        if (!ip)
            ip = unknown;


        JS_ObjectSetString(ctx, obj, "remote_address", ip);
        JS_ObjectSetNumber(ctx, obj, "remote_port", port);
        JS_ObjectSetNumber(ctx, obj, "remote_family", addr.ss_family);
    }

    peerlen = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &peerlen))
        JS_RETURN_OBJECT(ctx, obj);

    ip = 0;
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        port = ntohs(s->sin_port);
        ip   = inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else
    if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        port = ntohs(s->sin6_port);
        ip   = inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }
    if (!ip)
        ip = unknown;

    JS_ObjectSetString(ctx, obj, "local_address", ip);
    JS_ObjectSetNumber(ctx, obj, "local_port", port);
    JS_ObjectSetNumber(ctx, obj, "local_family", addr.ss_family);

    JS_RETURN_OBJECT(ctx, obj);
}


static void io_get_object(JS_CONTEXT ctx, int fd, const char *method) {
    char func_buffer[20]; 
    snprintf(func_buffer, sizeof(func_buffer), "io%u", fd); 

#ifdef WITH_DUKTAPE
    duk_push_global_stash(ctx);
        duk_get_prop_string(ctx, -1, func_buffer);
        if (duk_is_object(ctx, -1)) {
            duk_get_prop_string(ctx, -1, method);
            if (duk_is_function(ctx, -1)) {
                duk_call(ctx, 0);
                duk_pop(ctx);
            }
            duk_pop(ctx);
        }
    duk_pop(ctx);
#else
    js_object_type obj = JS_GetPropertyStr(ctx, global_stash(ctx), func_buffer);
    if (JS_IsObject(obj)) {
        js_object_type function_obj = JS_GetPropertyStr(ctx, obj, method);
        if (JS_IsFunction(ctx, function_obj))
            JS_FreeValueCheckException(ctx, JS_Call(ctx, function_obj, obj, 0, NULL));
    }
    JS_FreeValue(ctx, obj);
#endif
}

static void io_read_callback(struct doops_loop *loop, int fd) {
    JS_CONTEXT ctx = (JS_CONTEXT )loop_event_data(loop);
    if (!ctx)
        ctx = js();
    io_get_object(ctx, fd, "read");
}

static void io_write_callback(struct doops_loop *loop, int fd) {
    JS_CONTEXT ctx = (JS_CONTEXT )loop_event_data(loop);
    if (!ctx)
        ctx = js();
    io_get_object(ctx, fd, "write");
}

JS_C_FUNCTION(js_poll) {
    char func_buffer[20]; 

    JS_ParameterNumber(ctx, 0);
    JS_ParameterNumber(ctx, 1);
    JS_ParameterObject(ctx, 2);

    int fd = JS_GetIntParameter(ctx, 0);
    int err = loop_add_io_data(js_loop(), fd, JS_GetIntParameter(ctx, 1), ctx);
    if (!err) {
        if (!io_set) {
            loop_io(js_loop(), io_read_callback, io_write_callback);
            io_set = 1;
        }
        snprintf(func_buffer, sizeof(func_buffer), "io%u", fd); 
#ifdef WITH_DUKTAPE
        duk_push_global_stash(ctx);
            duk_dup(ctx, 2);
            duk_put_prop_string(ctx, -2, func_buffer);
        duk_pop_2(ctx);
#else
        JS_ObjectSetObject(ctx, global_stash(ctx), func_buffer, JS_DupValue(ctx, JS_GetObjectParameter(ctx, 2)));
#endif
    }
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_unpoll) {
    char func_buffer[20]; 

    JS_ParameterNumber(ctx, 0);
    int fd = JS_GetIntParameter(ctx, 0);

    int err = loop_remove_io(js_loop(), fd);
    if (!err) {
        snprintf(func_buffer, sizeof(func_buffer), "io%u", fd);
#ifdef WITH_DUKTAPE
        duk_push_global_stash(ctx);
        duk_push_string(ctx, func_buffer);
        duk_del_prop(ctx, -2);
        duk_pop_2(ctx);
#else
        JSAtom prop = JS_NewAtom(ctx, func_buffer);
        JS_DeleteProperty(ctx, global_stash(ctx), prop, 0);
        JS_FreeAtom(ctx, prop);
#endif
    }
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_poll_pause_write) {
    JS_ParameterNumber(ctx, 0);
    int err = loop_pause_write_io(js_loop(), JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_poll_resume_write) {
    JS_ParameterNumber(ctx, 0);
    int err = loop_resume_write_io(js_loop(), JS_GetIntParameter(ctx, 0));
    JS_RETURN_NUMBER(ctx, err);
}

#ifndef NO_TLS
JS_C_FUNCTION(js_tls_create) {
    unsigned char is_server = 0;
    int n = JS_ParameterCount(ctx);
    if (n > 0) {
        JS_ParameterBoolean(ctx, 0);
        is_server = JS_GetBooleanParameter(ctx, 0);
    }

    struct TLSContext *context = tls_create_context(is_server, TLS_V13);
    if (is_server) {
        if (n > 1) {
            JS_ParameterString(ctx, 1);
            js_size_t pem_size = 0;
            const char *pem_buffer = JS_GetStringParameterLen(ctx, 1, &pem_size);
            tls_load_certificates(context, (const unsigned char *)pem_buffer, pem_size);
        }
        if (n > 2) {
            JS_ParameterString(ctx, 2);
            js_size_t pem_size = 0;
            const char *pem_buffer = JS_GetStringParameterLen(ctx, 2, &pem_size);
            tls_load_private_key(context, (const unsigned char *)pem_buffer, pem_size);
        }
    } else {
        if (n > 1) {
            JS_ParameterString(ctx, 1);
            js_size_t pem_size = 0;
            const char *pem_buffer = JS_GetStringParameterLen(ctx, 1, &pem_size);
            tls_load_root_certificates(context, (const unsigned char *)pem_buffer, pem_size);
        }
    }
    JS_RETURN_POINTER(ctx, context);
}

JS_C_FUNCTION(js_tls_connect) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NUMBER(ctx, -1);
   
    int err = tls_client_connect(context);
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_tls_consume_stream) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_BOOLEAN(ctx, 0);

    int established = tls_established(context);
    JS_RETURN_BOOLEAN(ctx, established);
}

JS_C_FUNCTION(js_tls_established) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_BOOLEAN(ctx, 0);

    int established = tls_established(context);
    JS_RETURN_BOOLEAN(ctx, established);
}

JS_C_FUNCTION(js_tls_accept) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);

    struct TLSContext *context2 = tls_accept(context);
    if (context2)
        JS_RETURN_POINTER(ctx, context2);

    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tls_get_write_buffer) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);

    unsigned int outlen = 0;
    const unsigned char *buffer = tls_get_write_buffer(context, &outlen);
    if ((!outlen) || (!buffer))
        JS_RETURN_NOTHING(ctx);
#ifdef WITH_DUKTAPE
    char *js_buf = duk_push_fixed_buffer(ctx, outlen);
    if (!js_buf)
        JS_RETURN_NOTHING(ctx);

    memcpy(js_buf, buffer, outlen);
    duk_push_buffer_object(ctx, -1, 0, outlen, DUK_BUFOBJ_NODEJS_BUFFER);
    tls_buffer_clear(context);

    return 1;
#else
    JSValue val = JS_NewArrayBufferCopy(ctx, (const uint8_t *)buffer, outlen);
    tls_buffer_clear(context);

    return val;
#endif
}

JS_C_FUNCTION(js_tls_write) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NUMBER(ctx, -1);

    void *buf;
    js_size_t sz;

    buf = JS_GetBufferParameter(ctx, 1, &sz);

    ssize_t offset = 0;
    ssize_t nbytes = sz;
    int n = JS_ParameterCount(ctx);

    if (n > 2) {
        JS_ParameterNumber(ctx, 2);
        nbytes = (ssize_t)JS_GetNumberParameter(ctx, 2);
        if (nbytes < 0)
            JS_RETURN_NUMBER(ctx, 0);
    }
    if (n > 3) {
        JS_ParameterNumber(ctx, 3);
        offset = (ssize_t)JS_GetNumberParameter(ctx, 3);
        if (offset < 0)
            offset = 0;
    }
    if (offset >= sz)
        JS_RETURN_NUMBER(ctx, 0);

    if (offset + nbytes >= sz)
        nbytes = sz - offset;

    ssize_t bytes_written = tls_write(context, (unsigned char *)buf + offset, nbytes);
    JS_RETURN_NUMBER(ctx, bytes_written);
}

JS_C_FUNCTION(js_tls_read) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NUMBER(ctx, -1);

    JS_ParameterNumber(ctx, 2);

    js_size_t sz = 0;
    void *buf = JS_GetBufferParameter(ctx, 1, &sz);
    ssize_t nbytes = (ssize_t)JS_GetNumberParameter(ctx, 2);
    if (nbytes < 0)
        nbytes = 0;
    ssize_t offset = 0;

    if (JS_ParameterCount(ctx) > 3) {
        JS_ParameterNumber(ctx, 3);
        offset = (ssize_t)JS_GetNumberParameter(ctx, 3);
        if (offset < 0)
            offset = 0;
    }

    if (offset >= sz)
        JS_RETURN_NUMBER(ctx, 0);

    if (sz < (offset + nbytes))
        nbytes = sz - offset;
    
    ssize_t bytes_read = tls_read(context, (unsigned char *)buf + offset, nbytes);
    JS_RETURN_NUMBER(ctx, bytes_read);
}

JS_C_FUNCTION(js_tls_read_clear) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);
   
    tls_read_clear(context);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tls_close_notify) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);

    tls_close_notify(context);
    JS_RETURN_NOTHING(ctx);
}

JS_C_FUNCTION(js_tls_is_broken) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_BOOLEAN(ctx, 1);

    int is_broken = tls_is_broken(context);
    JS_RETURN_BOOLEAN(ctx, is_broken);
}

JS_C_FUNCTION(js_tls_alpn) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);

    JS_RETURN_STRING(ctx, tls_alpn(context));
}

JS_C_FUNCTION(js_tls_sni) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);

    JS_RETURN_STRING(ctx, tls_sni(context));
}

JS_C_FUNCTION(js_tls_sni_set) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);

    int err = tls_sni_set(context, JS_GetStringParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_tls_add_alpn) {
    JS_ParameterPointer(ctx, 0);
    JS_ParameterString(ctx, 1);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);

    int err = tls_add_alpn(context, JS_GetStringParameter(ctx, 1));
    JS_RETURN_NUMBER(ctx, err);
}

JS_C_FUNCTION(js_tls_cipher_name) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);
    JS_RETURN_STRING(ctx, tls_cipher_name(context));
}

JS_C_FUNCTION(js_tls_destroy_context) {
    JS_ParameterPointer(ctx, 0);
    struct TLSContext *context = (struct TLSContext *)JS_GetPointerParameter(ctx, 0);
    if (!context)
        JS_RETURN_NOTHING(ctx);
    JS_RETURN_NOTHING(ctx);
}

#endif

void register_socket_functions(void *main_loop, void *js_ctx) {
    JS_CONTEXT ctx = (JS_CONTEXT )js_ctx;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    register_object(ctx, "_net_",
        "socket", js_socket,
        "connect", js_connect,
        "bind", js_bind,
        "listen", js_listen,
        "accept", js_accept,
        "recv", js_recv,
        "send", js_send,
        "recvfrom", js_recvfrom,
        "sendto", js_sendto,
        "close", js_closesocket,
        "shutdown", js_shutdown,
        "setReuseAddr", js_setreuseaddr,
        "setNoDelay", js_setnodelay,
        "setTimeout", js_settimeout,
        "setKeepAlive", js_setkeepalive,
        "setBlocking", js_setblocking,
        "info", js_socketinfo,
        "errno", js_socketerrno,
        "strerror", js_socketstrerror,
        "getError", js_geterror,
        "poll", js_poll,
        "unpoll", js_unpoll,
        "pollPauseWrite", js_poll_pause_write,
        "pollResumeWrite", js_poll_resume_write,
        NULL, NULL
    );
#ifndef NO_TLS
    register_object(ctx, "_tls_",
        "create", js_tls_create,
        "connect", js_tls_connect,
        "consume", js_tls_consume_stream,
        "established", js_tls_established,
        "accept", js_tls_accept,
        "getWriteBuffer", js_tls_get_write_buffer,
        "write", js_tls_write,
        "read", js_tls_read,
        "readClear", js_tls_read_clear,
        "close", js_tls_close_notify,
        "alpn", js_tls_alpn,
        "addAlpn", js_tls_add_alpn,
        "sni", js_tls_sni,
        "setSni", js_tls_sni_set,
        "cipher", js_tls_cipher_name,
        "destroy", js_tls_destroy_context,
        "isBroken", js_tls_is_broken,
        NULL, NULL
    );
#endif
}
