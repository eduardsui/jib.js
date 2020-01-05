#ifndef DOOPS_H
#define DOOPS_H

#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>

#ifdef _WIN32
    #define WITH_SELECT
    #define DOOPS_SPINLOCK_TYPE LONG
#else
    #define DOOPS_SPINLOCK_TYPE int
#ifdef __linux__
    #define WITH_EPOLL
#else
#if defined(__MACH__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #define WITH_KQUEUE
#else
#ifndef DOOPS_NO_IO_EVENTS
    #pragma message ( "WARNING: Cannot determine operating system. Falling back to select." )
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/select.h>
#endif
    #define WITH_SELECT
#endif
#endif
#endif

#ifdef WITH_EPOLL
    #include <sys/epoll.h>
#endif
#ifdef WITH_KQUEUE
    #include <sys/types.h>
    #include <sys/event.h>
#endif

#define DOOPS_MAX_SLEEP     500
#define DOOPS_MAX_EVENTS    1024

#if !defined(DOOPS_FREE) || !defined(DOOPS_MALLOC) || !defined(DOOPS_REALLOC)
    #define DOOPS_MALLOC(bytes)         malloc(bytes)
    #define DOOPS_FREE(ptr)             free(ptr)
    #define DOOPS_REALLOC(ptr, size)    realloc(ptr, size)
#endif

#ifdef _WIN32
    #include <windows.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
        #define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64
    #else
        #define DELTA_EPOCH_IN_MICROSECS  116444736000000000ULL
    #endif

    static int gettimeofday(struct timeval *tv, void *not_used) {
        FILETIME         ft;
        unsigned __int64 tmpres = 0;

        if (NULL != tv) {
            GetSystemTimeAsFileTime(&ft);

            tmpres  |= ft.dwHighDateTime;
            tmpres <<= 32;
            tmpres  |= ft.dwLowDateTime;

            tmpres     -= DELTA_EPOCH_IN_MICROSECS;
            tmpres     /= 10; 

            tv->tv_sec  = (long)(tmpres / 1000000UL);
            tv->tv_usec = (long)(tmpres % 1000000UL);
        }
        return 0;
    }
#else
    #include <sys/time.h>
    #include <unistd.h>
#endif

#define DOOPS_READ      0
#define DOOPS_READWRITE 1

struct doops_loop;

#ifdef __cplusplus
    #define PRIVATE_LOOP_MAKE_ANON_FUNCTION(x, y) private_lambda_call_ ## x ## y
    #define PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(x, y) PRIVATE_LOOP_MAKE_ANON_FUNCTION(x, y)

    #define PRIVATE_LOOP_MAKE_ANON_STRUCT(x, y) private_lambda_struct_ ## x ## y
    #define PRIVATE_LOOP_MAKE_ANON_FUNCTION_STRUCT(x, y) PRIVATE_LOOP_MAKE_ANON_STRUCT(x, y)

    #define loop_code_data(loop_ptr, code, interval, userdata_ptr) { \
        struct PRIVATE_LOOP_MAKE_ANON_STRUCT(__func__, __LINE__) { \
            static int PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__) (struct doops_loop *loop) { \
                code; \
                return 0; \
            }; \
        }; \
        loop_add(loop_ptr, PRIVATE_LOOP_MAKE_ANON_STRUCT(__func__, __LINE__)::PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__), interval, userdata_ptr); \
    }

    #define loop_on_read(loop_ptr, code) { \
        struct PRIVATE_LOOP_MAKE_ANON_STRUCT(__func__, __LINE__) { \
            static void PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__) (struct doops_loop *loop, int fd) { \
                code; \
            }; \
        }; \
        if (loop_ptr) \
            (loop_ptr)->io_read = PRIVATE_LOOP_MAKE_ANON_STRUCT(__func__, __LINE__)::PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__); \
    }

    #define loop_on_write(loop_ptr, code) { \
        struct PRIVATE_LOOP_MAKE_ANON_STRUCT(__func__, __LINE__) { \
            static void PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__) (struct doops_loop *loop, int fd) { \
                code; \
            }; \
        }; \
        if (loop_ptr) \
            (loop_ptr)->io_write = PRIVATE_LOOP_MAKE_ANON_STRUCT(__func__, __LINE__)::PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__); \
    }

    #define LOOP_IS_READABLE(loop) (loop->io_read)
    #define LOOP_IS_WRITABLE(loop) (loop->io_write)
#else
#ifdef __clang__
    #define WITH_BLOCKS

    #include <Block.h>

    #define loop_code_data(loop_ptr, code, interval, userdata_ptr) { \
        loop_add_block(loop_ptr, ^(struct doops_loop *loop) { \
            code; \
            return 0; \
        }, interval, userdata_ptr); \
    }

    #define loop_on_read(loop_ptr, code) { \
        if (loop_ptr) { \
            (loop_ptr)->io_read = NULL; \
            if ((loop_ptr)->io_read_block) \
                Block_release((loop_ptr)->io_read_block); \
            (loop_ptr)->io_read_block = Block_copy(^(struct doops_loop *loop, int fd) { code; }); \
        } \
    }

    #define loop_on_write(loop_ptr, code) { \
        if (loop_ptr) { \
            (loop_ptr)->io_write = NULL; \
            if ((loop_ptr)->io_write_block) \
                Block_release((loop_ptr)->io_write_block); \
            (loop_ptr)->io_write_block = Block_copy(^(struct doops_loop *loop, int fd) { code; }); \
        } \
    }

    #define LOOP_IS_READABLE(loop) ((loop->io_read) || (loop->io_read_block))
    #define LOOP_IS_WRITABLE(loop) ((loop->io_write) || (loop->io_write_block))
#else
    #define LOOP_IS_READABLE(loop) (loop->io_read)
    #define LOOP_IS_WRITABLE(loop) (loop->io_write)

    #ifdef __GNUC__
        #define PRIVATE_LOOP_MAKE_ANON_FUNCTION(x, y) private_lambda_call_ ## x ## y
        #define PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(x, y) PRIVATE_LOOP_MAKE_ANON_FUNCTION(x, y)

        #define loop_code_data(loop_ptr, code, interval, userdata_ptr) { \
            int PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__) (struct doops_loop *loop) { \
                code; \
                return 0; \
            }; \
            loop_add(loop_ptr, PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__), interval, userdata_ptr); \
        }

        #define loop_on_read(loop_ptr, code) { \
            void PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__) (struct doops_loop *loop, int fd) { \
                code; \
            }; \
            if (loop_ptr) \
                (loop_ptr)->io_read = PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__); \
        }

        #define loop_on_write(loop_ptr, code) { \
            void PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__) (struct doops_loop *loop, int fd) { \
                code; \
            }; \
            if (loop_ptr) \
                (loop_ptr)->io_write = PRIVATE_LOOP_MAKE_ANON_FUNCTION_NAME(__func__, __LINE__); \
        }
    #else
        #pragma message ( "Code blocks are not supported by your compiler" )
    #endif
#endif
#endif

#define loop_code(loop_ptr, code, interval) loop_code_data(loop_ptr, code, interval, NULL);
#define loop_schedule                       loop_code

typedef int (*doop_callback)(struct doops_loop *loop);
typedef int (*doop_foreach_callback)(struct doops_loop *loop, void *foreachdata);
typedef int (*doop_idle_callback)(struct doops_loop *loop);
typedef void (*doop_io_callback)(struct doops_loop *loop, int fd);
typedef void (*doop_udata_free_callback)(struct doops_loop *loop, void *ptr);

#ifdef WITH_BLOCKS
    typedef int (^doop_callback_block)(struct doops_loop *loop);
    typedef void (^doop_io_callback_block)(struct doops_loop *loop, int fd);
#endif

struct doops_event {
    doop_callback event_callback;
#ifdef WITH_BLOCKS
    doop_callback_block event_block;
#endif
    uint64_t when;
    uint64_t interval;
    void *user_data;
    struct doops_event *next;
};

struct doops_loop {
    int quit;
    doop_idle_callback idle;
    struct doops_event *events;
    doop_io_callback io_read;
    doop_io_callback io_write;
    doop_udata_free_callback udata_free;
#ifdef WITH_BLOCKS
    doop_io_callback_block io_read_block;
    doop_io_callback_block io_write_block;
#endif
#if defined(WITH_EPOLL) || defined(WITH_KQUEUE)
    int poll_fd;
    int max_fd;
    void **udata;
#else
    int max_fd;
    // fallback to select
    fd_set inlist;
    fd_set outlist;
    fd_set exceptlist;
    void **udata;
#endif
    DOOPS_SPINLOCK_TYPE lock;
    int event_fd;
    unsigned int io_objects;
    void *event_data;
    struct doops_event *in_event;
    unsigned char reset_in_event;
};

static void _private_loop_init_io(struct doops_loop *loop) {
    if (!loop)
        return;

#ifdef WITH_EPOLL
    if (loop->poll_fd <= 0)
        loop->poll_fd = epoll_create1(0);
#else
#ifdef WITH_KQUEUE
    if (loop->poll_fd <= 0)
        loop->poll_fd = kqueue();
#else
    if (!loop->max_fd) {
        FD_ZERO(&loop->inlist);
        FD_ZERO(&loop->outlist);
        FD_ZERO(&loop->exceptlist);
        loop->max_fd = 1;
    }
#endif
#endif
}

static uint64_t milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

static void doops_lock(volatile DOOPS_SPINLOCK_TYPE *ptr) {
    if (!ptr)
        return;
#ifdef _WIN32
    while (InterlockedCompareExchange(ptr, 1, 0)) {
        // wait for lock
    }
#else
    while (__sync_lock_test_and_set(ptr, 1))
        while (*ptr);
#endif
}

static void doops_unlock(volatile DOOPS_SPINLOCK_TYPE *ptr) {
    if (!ptr)
        return;
#ifdef _WIN32
    InterlockedExchange(ptr, 0);
#else
    __sync_lock_release (ptr);
#endif
}

static void loop_init(struct doops_loop *loop) {
    if (loop)
        memset(loop, 0, sizeof(struct doops_loop));
}

static struct doops_loop *loop_new() {
    struct doops_loop *loop = (struct doops_loop *)DOOPS_MALLOC(sizeof(struct doops_loop));
    if (loop)
        loop_init(loop);
    return loop;
}

static int loop_add(struct doops_loop *loop, doop_callback callback, int64_t interval, void *user_data) {
    if ((!callback) || (!loop)) {
        errno = EINVAL;
        return -1;
    }

    struct doops_event *event_callback = (struct doops_event *)DOOPS_MALLOC(sizeof(struct doops_event));
    if (!event_callback) {
        errno = ENOMEM;
        return -1;
    }

    if (!loop->in_event)
        doops_lock(&loop->lock);
    event_callback->event_callback = callback;
#ifdef WITH_BLOCKS
    event_callback->event_block = NULL;
#endif
    if (interval < 0)
        event_callback->interval = (uint64_t)(-interval);
    else
        event_callback->interval = (uint64_t)interval;
    event_callback->when = milliseconds() + interval;
    event_callback->user_data = user_data;
    event_callback->next = loop->events;

    loop->events = event_callback;
    if (loop->in_event)
        loop->reset_in_event = 2;
    else
        doops_unlock(&loop->lock);
    return 0;
}

#ifdef WITH_BLOCKS
static int loop_add_block(struct doops_loop *loop, doop_callback_block callback, int64_t interval, void *user_data) {
    if ((!callback) || (!loop)) {
        errno = EINVAL;
        return -1;
    }

    struct doops_event *event_callback = (struct doops_event *)DOOPS_MALLOC(sizeof(struct doops_event));
    if (!event_callback) {
        errno = ENOMEM;
        return -1;
    }

    if (!loop->in_event)
        doops_lock(&loop->lock);
    event_callback->event_callback = NULL;
    event_callback->event_block = Block_copy(callback);
    if (interval < 0)
        event_callback->interval = (uint64_t)(-interval);
    else
        event_callback->interval = (uint64_t)interval;
    event_callback->when = milliseconds() + interval;
    event_callback->user_data = user_data;
    event_callback->next = loop->events;

    loop->events = event_callback;
    if (loop->in_event)
        loop->reset_in_event = 2;
    else
        doops_unlock(&loop->lock);
    return 0;
}
#endif

static int loop_add_io_data(struct doops_loop *loop, int fd, int mode, void *userdata) {
    if ((fd < 0) || (!loop)) {
        errno = EINVAL;
        return -1;
    }
    doops_lock(&loop->lock);
    _private_loop_init_io(loop);
#ifdef WITH_EPOLL
    struct epoll_event event;
    // supress valgrind warning
    event.data.u64 = 0;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLPRI | EPOLLHUP | EPOLLRDHUP | EPOLLET;
    if (mode)
        event.events |= EPOLLOUT;

    int err = epoll_ctl (loop->poll_fd, EPOLL_CTL_ADD, fd, &event);
    if ((err) && (errno == EEXIST))
        err = epoll_ctl (loop->poll_fd, EPOLL_CTL_MOD, fd, &event);
    if (err) {
        doops_unlock(&loop->lock);
        return -1;
    }
    if ((userdata) || (loop->udata)) {
        if (fd >= loop->max_fd) {
            loop->max_fd = fd + 1;
            loop->udata = DOOPS_REALLOC(loop->udata, sizeof(void *) * loop->max_fd);
        }
        if (loop->udata)
            loop->udata[fd] = userdata;
    }
#else
#ifdef WITH_KQUEUE
    struct kevent events[2];
    int num_events = 1;
    EV_SET(&events[0], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
    events[0].udata = userdata;
    if (mode) {
        EV_SET(&events[1], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, 0);
        events[1].udata = userdata;
        num_events = 2;
    }
    doops_unlock(&loop->lock);
    return kevent(loop->poll_fd, events, num_events, NULL, 0, NULL);
#else
    FD_SET(fd, &loop->inlist);
    FD_SET(fd, &loop->exceptlist);
    if (mode)
        FD_SET(fd, &loop->outlist);

    if (fd >= loop->max_fd)
        loop->max_fd = fd + 1;

    if ((userdata) || (loop->udata)) {
        loop->udata = (void **)DOOPS_REALLOC(loop->udata, sizeof(void *) * loop->max_fd);
        if (loop->udata)
            loop->udata[fd] = userdata;
    }
#endif
#endif
    loop->io_objects ++;
    doops_unlock(&loop->lock);
    return 0;
}

static int loop_add_io(struct doops_loop *loop, int fd, int mode) {
    return loop_add_io_data(loop, fd, mode, NULL);
}

static int loop_pause_write_io(struct doops_loop *loop, int fd) {
    if (!loop) {
        errno = EINVAL;
        return -1;
    }
#ifdef WITH_KQUEUE
    // using EV_CLEAR instead
    // struct kevent event;
    // EV_SET(&event, fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, 0);
    // return kevent(loop->poll_fd, &event, 1, NULL, 0, NULL);
#else
#ifdef WITH_SELECT
    FD_CLR(fd, &loop->outlist);
#endif
#endif
    return 0;
}

static int loop_resume_write_io(struct doops_loop *loop, int fd) {
    if (!loop) {
        errno = EINVAL;
        return -1;
    }
#ifdef WITH_KQUEUE
    // using EV_CLEAR instead
    // struct kevent event;
    // EV_SET(&event, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
    // return kevent(loop->poll_fd, &event, 1, NULL, 0, NULL);
#else
#ifdef WITH_SELECT
    FD_SET(fd, &loop->outlist);
#endif
#endif
    return 0;
}

static int loop_remove_io(struct doops_loop *loop, int fd) {
    if ((fd < 0) || (!loop)) {
        errno = EINVAL;
        return -1;
    }
    _private_loop_init_io(loop);
#ifdef WITH_EPOLL
    struct epoll_event event;
    // supress valgrind warning
    event.data.u64 = 0;
    event.data.fd = fd;
    event.events = 0;
    if (fd == loop->max_fd - 1)
        loop->max_fd --;
    int err = epoll_ctl (loop->poll_fd, EPOLL_CTL_DEL, fd, &event);
    if (!err)
        loop->io_objects --;
    return err;
#else
#ifdef WITH_KQUEUE
    struct kevent event;
    EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
    kevent(loop->poll_fd, &event, 1, NULL, 0, NULL);
    EV_SET(&event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
    kevent(loop->poll_fd, &event, 1, NULL, 0, NULL);
#else
    FD_CLR(fd, &loop->inlist);
    FD_CLR(fd, &loop->exceptlist);
    FD_CLR(fd, &loop->outlist);
    if (fd == loop->max_fd - 1)
        loop->max_fd --;
#endif
#endif
    loop->io_objects --;
    return 0;
}

static int loop_remove(struct doops_loop *loop, doop_callback callback, void *user_data) {
    if (!loop) {
        errno = EINVAL;
        return -1;
    }
    int locked = 0;
    if (!loop->in_event) {
        doops_lock(&loop->lock);
        locked = 1;
    } else
    if (!loop->reset_in_event)
        loop->reset_in_event = 2;
    int removed_event = 0;
    if ((loop->events) && (!loop->quit)) {
        struct doops_event *ev = loop->events;
        struct doops_event *prev_ev = NULL;
        struct doops_event *next_ev = NULL; 
        void *userdata = loop->event_data;
        while (ev) {
            next_ev = ev->next;
            if (((!callback) || (callback == ev->event_callback)) && ((!user_data) || (user_data == ev->user_data))) {
                if (loop->in_event == ev) {
                    // cannot delete current event, notify the loop
                    loop->reset_in_event = 1;
                } else {
                    if ((loop->udata_free) && (loop->events->user_data)) {
                        loop->event_data = loop->events->user_data;
                        loop->udata_free(loop, loop->events->user_data);
                    }
                    DOOPS_FREE(ev);
                    if (prev_ev)
                        prev_ev->next = next_ev;
                    else
                        loop->events = next_ev;
                    ev = next_ev;
                    if (ev)
                        next_ev = ev->next;
                }
                removed_event ++;
                if ((callback) && (user_data))
                    break;
                continue;
            }
            prev_ev = ev;
            ev = next_ev;
        }
        loop->event_data = userdata;
    }
    if (locked)
        doops_unlock(&loop->lock);
    return removed_event;
}

static int loop_foreach_callback(struct doops_loop *loop, void *foreachcallback, doop_foreach_callback callback, void *foreachdata) {
    if ((!loop) || (!callback)) {
        errno = EINVAL;
        return -1;
    }
    doops_lock(&loop->lock);
    int removed_event = 0;
    if ((loop->events) && (!loop->quit)) {
        struct doops_event *ev = loop->events;
        struct doops_event *prev_ev = NULL;
        struct doops_event *next_ev = NULL; 
        void *userdata = loop->event_data;
        while ((ev) && (!loop->quit)) {
            next_ev = ev->next;
            if ((ev->event_callback == foreachcallback) || (!foreachcallback)) {
                loop->event_data = ev->user_data;
                int ret_code = callback(loop, foreachdata);
                if (ret_code < 0)
                    break;
                if (ret_code) {
                    if ((loop->udata_free) && (ev->user_data))
                        loop->udata_free(loop, ev->user_data);
#ifdef WITH_BLOCKS
                    if (ev->event_block)
                        Block_release(ev->event_block);
#endif
                    DOOPS_FREE(ev);
                    if (prev_ev)
                        prev_ev->next = next_ev;
                    else
                        loop->events = next_ev;
                    ev = next_ev;
                    removed_event ++;
                    if (ret_code != 1)
                        continue;
                }
            }
            prev_ev = ev;
            ev = next_ev;
        }
        loop->event_data = userdata;
    }
    doops_unlock(&loop->lock);
    return removed_event;
}

static int loop_foreach(struct doops_loop *loop, doop_foreach_callback callback, void *foreachdata) {
    return loop_foreach_callback(loop, NULL, callback, foreachdata);
}

static void loop_quit(struct doops_loop *loop) {
    if (loop)
        loop->quit = 1;
}

static int _private_loop_iterate(struct doops_loop *loop, int *sleep_val) {
    int loops = 0;
    if (sleep_val)
        *sleep_val = DOOPS_MAX_SLEEP;
    doops_lock(&loop->lock);
    if ((loop->events) && (!loop->quit)) {
        struct doops_event *ev = loop->events;
        struct doops_event *prev_ev = NULL;
        struct doops_event *next_ev = NULL; 
        while ((ev) && (!loop->quit)) {
            next_ev = ev->next;
            uint64_t now = milliseconds();
            if (ev->when <= now) {
                loops ++;
                loop->event_data = ev->user_data;
                int remove_event = 1;
                loop->in_event = ev;
                loop->reset_in_event = 0;
#ifdef WITH_BLOCKS
                if (ev->event_block)
                    remove_event = ev->event_block(loop);
                else
#endif
                if (ev->event_callback)
                    remove_event = ev->event_callback(loop);
                // remove_event called on the current event
                if (loop->reset_in_event) {
                    next_ev = ev->next;
                    prev_ev = NULL;
                    struct doops_event *ev_2 = loop->events;
                    while ((ev_2) && (ev != ev_2)) {
                        prev_ev = ev_2;
                        ev_2 = ev_2->next;
                    }

                    if (loop->reset_in_event == 1)
                        remove_event = 1;
                    loop->reset_in_event = 0;

                    *sleep_val = 0;
                }
                loop->in_event = NULL;
                if (remove_event) {
                    if ((loop->udata_free) && (ev->user_data))
                        loop->udata_free(loop, ev->user_data);
#ifdef WITH_BLOCKS
                    if (ev->event_block)
                        Block_release(ev->event_block);
#endif
                    DOOPS_FREE(ev);
                    if (prev_ev)
                        prev_ev->next = next_ev;
                    else
                        loop->events = next_ev;
                    ev = next_ev;
                    continue;
                }
                while ((ev->when <= now) && (ev->interval))
                    ev->when += ev->interval;
            }
            if (sleep_val) {
                int delta = (int)(ev->when - now);
                if (delta < *sleep_val)
                    *sleep_val = delta;
            }
            prev_ev = ev;
            ev = next_ev;
        }
        if (!loop->events)
            *sleep_val = 0;
    }
    doops_unlock(&loop->lock);
    return loops;
}

static int loop_iterate(struct doops_loop *loop) {
    return _private_loop_iterate(loop, NULL);
}

static int loop_idle(struct doops_loop *loop, doop_idle_callback callback) {
    if (!loop) {
        errno = EINVAL;
        return -1;
    }
    loop->idle = callback;
    return 0;
}

static int loop_io(struct doops_loop *loop, doop_io_callback read_callback, doop_io_callback write_callback) {
    if (!loop) {
        errno = EINVAL;
        return -1;
    }
    loop->io_read = read_callback;
    loop->io_write = write_callback;
#ifdef WITH_BLOCK
    if (loop->io_read_block) {
        Block_release(loop->io_read_block);
        loop->io_read_block = NULL;
    }
    if (loop->io_write_block) {
        Block_release(loop->io_write_block);
        loop->io_write_block = NULL;
    }
#endif
    return 0;
}

static void _private_loop_remove_events(struct doops_loop *loop) {
    struct doops_event *next_ev;
    doops_lock(&loop->lock);
    while (loop->events) {
        next_ev = loop->events->next;
        if ((loop->udata_free) && (loop->events->user_data)) {
            loop->event_data = loop->events->user_data;
            loop->udata_free(loop, loop->events->user_data);
        }
#ifdef WITH_BLOCKS
        if ((loop->events) && (loop->events->event_block))
            Block_release(loop->events->event_block);
#endif
        DOOPS_FREE(loop->events);
        loop->events = next_ev;
    }
#ifndef WITH_KQUEUE
    if (loop->udata) {
        DOOPS_FREE(loop->udata);
        loop->udata = NULL;
    }
    loop->max_fd = 1;
#endif
    doops_unlock(&loop->lock);
}

static void _private_sleep(struct doops_loop *loop, int sleep_val) {
    if (!loop)
        return;
#ifndef DOOPS_NO_IO_EVENTS
#ifdef WITH_EPOLL
    if ((loop->poll_fd > 0) && ((LOOP_IS_READABLE(loop)) || (LOOP_IS_WRITABLE(loop)))) {
        struct epoll_event events[DOOPS_MAX_EVENTS];
        int nfds = epoll_wait(loop->poll_fd, events, DOOPS_MAX_EVENTS, sleep_val);
        int i;
        for (i = 0; i < nfds; i ++) {
            if (LOOP_IS_WRITABLE(loop)) {
                if (events[i].events & EPOLLOUT) {
                    loop->event_fd = events[i].data.fd;
                    loop->event_data = loop->udata ? loop->udata[events[i].data.fd] : NULL;
#ifdef WITH_BLOCKS
                    if (loop->io_write_block)
                        loop->io_write_block(loop, events[i].data.fd);
                    else
#endif
                    loop->io_write(loop, events[i].data.fd);
                }
            }
            if (LOOP_IS_READABLE(loop)) {
                if (events[i].events ^ EPOLLOUT) {
                    loop->event_fd = events[i].data.fd;
                    loop->event_data = loop->udata ? loop->udata[events[i].data.fd] : NULL;
#ifdef WITH_BLOCKS
                    if (loop->io_read_block)
                        loop->io_read_block(loop, events[i].data.fd);
                    else
#endif
                    loop->io_read(loop, events[i].data.fd);
                }
            }
        }
    } else
#else
#ifdef WITH_KQUEUE
    if ((loop->poll_fd > 0) && ((LOOP_IS_READABLE(loop)) || (LOOP_IS_WRITABLE(loop)))) {
        struct kevent events[DOOPS_MAX_EVENTS];
        struct timespec timeout_spec;
        if (sleep_val >= 0) {
            timeout_spec.tv_sec = sleep_val / 1000;
            timeout_spec.tv_nsec = (sleep_val % 1000) * 1000;
        }
        int events_count = kevent(loop->poll_fd, NULL, 0, events, DOOPS_MAX_EVENTS, (sleep_val >= 0) ? &timeout_spec : NULL);
        int i;
        for (i = 0; i < events_count; i ++) {
            if (LOOP_IS_WRITABLE(loop)) {
                if (events[i].filter == EVFILT_WRITE) {
                    loop->event_fd = events[i].ident;
                    loop->event_data = events[i].udata;
#ifdef WITH_BLOCKS
                    if (loop->io_write_block)
                        loop->io_write_block(loop, events[i].ident);
                    else
#endif
                    loop->io_write(loop, events[i].ident);
                }
            }
            if (LOOP_IS_READABLE(loop)) {
                if (events[i].filter != EVFILT_WRITE) {
                    loop->event_fd = events[i].ident;
                    loop->event_data = events[i].udata;
#ifdef WITH_BLOCKS
                    if (loop->io_read_block)
                        loop->io_read_block(loop, events[i].ident);
                    else
#endif
                    loop->io_read(loop, events[i].ident);
                }
            }
        }
    } else
#else
    if ((loop->max_fd) && ((LOOP_IS_READABLE(loop)) || (LOOP_IS_WRITABLE(loop)))) {
        struct timeval tout;
        tout.tv_sec = 0;
        tout.tv_usec = 0;
        if (sleep_val > 0) {
            tout.tv_sec = sleep_val / 1000;
            tout.tv_usec = (sleep_val % 1000) * 1000;
        }
        fd_set inlist;
        fd_set outlist;
        fd_set exceptlist;

        // fd_set is a struct
        inlist = loop->inlist;
        outlist = loop->outlist;
        exceptlist = loop->exceptlist;
        int err = select(loop->max_fd, &inlist, &outlist, &exceptlist, &tout);
        if (err >= 0) {
            if (!err)
                return;
            int i;
            for (i = 0; i < loop->max_fd; i ++) {
                if (LOOP_IS_READABLE(loop)) {
                    if ((FD_ISSET(i, &inlist)) || (FD_ISSET(i, &exceptlist))) {
                        loop->event_fd = i;
                        loop->event_data = loop->udata ? loop->udata[i] : NULL;
#ifdef WITH_BLOCKS
                        if (loop->io_read_block)
                            loop->io_read_block(loop, i);
                        else
#endif
                        loop->io_read(loop, i);
                    }
                }
                if (LOOP_IS_WRITABLE(loop)) {
                    if (FD_ISSET(i, &outlist)) {
                        loop->event_fd = i;
                        loop->event_data = loop->udata ? loop->udata[i] : NULL;
#ifdef WITH_BLOCKS
                        if (loop->io_write_block)
                            loop->io_write_block(loop, i);
                        else
#endif
                        loop->io_write(loop, i);
                    }
                }
            }
        }
    } else
#endif
#endif
#endif
#ifdef _WIN32
    Sleep(sleep_val);
#else
    usleep(sleep_val * 1000);
#endif
}

static void loop_run(struct doops_loop *loop) {
    if (!loop)
        return;

    int sleep_val;
    while (((loop->events) || (loop->io_objects)) && (!loop->quit)) {
        loop->event_fd = -1;
        int loops = _private_loop_iterate(loop, &sleep_val);
        loop->event_data = NULL;
        if ((sleep_val > 0) && (!loops) && (loop->idle) && (loop->idle(loop)))
            break;
        _private_sleep(loop, sleep_val);
    }
    _private_loop_remove_events(loop);
    loop->quit = 1;
}

static void loop_deinit(struct doops_loop *loop) {
    if (loop) {
#if defined(WITH_EPOLL) || defined(WITH_KQUEUE)
        if (loop->poll_fd > 0) {
            close(loop->poll_fd);
            loop->poll_fd = -1;
        }
#else
        FD_ZERO(&loop->inlist);
        FD_ZERO(&loop->outlist);
        FD_ZERO(&loop->exceptlist);
#endif
        _private_loop_remove_events(loop);
#ifdef WITH_BLOCKS
        if (loop->io_read_block) {
            Block_release(loop->io_read_block);
            loop->io_read_block = NULL;
        }
        if (loop->io_write_block) {
            Block_release(loop->io_write_block);
            loop->io_write_block = NULL;
        }
#endif
    }
}

static void loop_free(struct doops_loop *loop) {
    loop_deinit(loop);
    DOOPS_FREE(loop);
}

static int loop_event_socket(struct doops_loop *loop) {
    if (loop)
        return loop->event_fd;
    return -1;
}

static void *loop_event_data(struct doops_loop *loop) {
    if (loop)
        return loop->event_data;
    return NULL;
}

#endif
