#include "poll.h"

// based on Paolo Bonzini poll-win32

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <assert.h>

#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <signal.h>

#include <time.h>

#ifndef POLLRDNORM
    #define POLLRDNORM  0
    #define POLLRDBAND  0
    #define POLLWRNORM  0
    #define POLLWRBAND  0
#endif

#define IsConsoleHandle(h) (((intptr_t) (h) & 3) == 3)

int set_blocking_mode(const int socket, int is_blocking) {
    int ret = 1;
    u_long flags = is_blocking ? 0 : 1;
    ret = NO_ERROR == ioctlsocket(socket, FIONBIO, &flags);
    return ret;
}

static BOOL IsSocketHandle(HANDLE h) {
    WSANETWORKEVENTS ev;

    if (IsConsoleHandle(h))
        return FALSE;

    ev.lNetworkEvents = 0xDEADBEEFl;
    WSAEnumNetworkEvents((SOCKET) h, NULL, &ev);
    return ev.lNetworkEvents != 0xDEADBEEFl;
}

static HANDLE HandleFromFd(int fd) {
    if (IsSocketHandle((HANDLE)((uintptr_t)fd)))
        return ((HANDLE)(uintptr_t)fd);

    return ((HANDLE) _get_osfhandle(fd));
}

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
    ULONG NamedPipeType;
    ULONG NamedPipeConfiguration;
    ULONG MaximumInstances;
    ULONG CurrentInstances;
    ULONG InboundQuota;
    ULONG ReadDataAvailable;
    ULONG OutboundQuota;
    ULONG WriteQuotaAvailable;
    ULONG NamedPipeState;
    ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _IO_STATUS_BLOCK {
    union {
        DWORD Status;
        PVOID Pointer;
    } u;
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _FILE_INFORMATION_CLASS {
    FilePipeLocalInformation = 24
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef DWORD (WINAPI *PNtQueryInformationFile)(HANDLE, IO_STATUS_BLOCK *, VOID *, ULONG, FILE_INFORMATION_CLASS);

#ifndef PIPE_BUF
    #define PIPE_BUF      512
#endif

static int windows_compute_revents(HANDLE h, int *p_sought) {
    int i, ret, happened;
    INPUT_RECORD *irbuffer;
    DWORD avail, nbuffer;
    BOOL bRet;
    IO_STATUS_BLOCK iosb;
    FILE_PIPE_LOCAL_INFORMATION fpli;
    static PNtQueryInformationFile NtQueryInformationFile;
    static BOOL once_only;

    switch (GetFileType (h)) {
        case FILE_TYPE_PIPE:
            if (!once_only) {
                NtQueryInformationFile = (PNtQueryInformationFile)GetProcAddress(GetModuleHandle ("ntdll.dll"), "NtQueryInformationFile");
                once_only = TRUE;
            }
            happened = 0;
            if (PeekNamedPipe (h, NULL, 0, NULL, &avail, NULL) != 0) {
                if (avail)
                    happened |= *p_sought & (POLLIN | POLLRDNORM);
            } else
            if (GetLastError () == ERROR_BROKEN_PIPE) {
                happened |= POLLHUP;
            } else {
                memset (&iosb, 0, sizeof (iosb));
                memset (&fpli, 0, sizeof (fpli));

                if (!NtQueryInformationFile || NtQueryInformationFile (h, &iosb, &fpli, sizeof (fpli), FilePipeLocalInformation) || fpli.WriteQuotaAvailable >= PIPE_BUF
                    || (fpli.OutboundQuota < PIPE_BUF && fpli.WriteQuotaAvailable == fpli.OutboundQuota))
                    happened |= *p_sought & (POLLOUT | POLLWRNORM | POLLWRBAND);
            }
            return happened;

        case FILE_TYPE_CHAR:
            ret = WaitForSingleObject (h, 0);
            if (!IsConsoleHandle (h))
                return ret == WAIT_OBJECT_0 ? *p_sought & ~(POLLPRI | POLLRDBAND) : 0;

            nbuffer = avail = 0;
            bRet = GetNumberOfConsoleInputEvents (h, &nbuffer);
            if (bRet) {
                *p_sought &= POLLIN | POLLRDNORM;
                if (nbuffer == 0)
                    return POLLHUP;
                if (!*p_sought)
                    return 0;

                irbuffer = (INPUT_RECORD *) alloca (nbuffer * sizeof (INPUT_RECORD));
                bRet = PeekConsoleInput (h, irbuffer, nbuffer, &avail);
                if (!bRet || avail == 0)
                    return POLLHUP;

                for (i = 0; i < avail; i++)
                    if (irbuffer[i].EventType == KEY_EVENT)
                        return *p_sought;
                return 0;
            } else {
                *p_sought &= POLLOUT | POLLWRNORM | POLLWRBAND;
                return *p_sought;
            }
        default:
            ret = WaitForSingleObject (h, 0);
            if (ret == WAIT_OBJECT_0)
                return *p_sought & ~(POLLPRI | POLLRDBAND);

            return *p_sought & (POLLOUT | POLLWRNORM | POLLWRBAND);
    }
}

static int windows_compute_revents_socket(SOCKET h, int sought, long lNetworkEvents) {
    int happened = 0;

    if ((lNetworkEvents & (FD_READ | FD_ACCEPT | FD_CLOSE)) == FD_ACCEPT) {
        happened |= (POLLIN | POLLRDNORM) & sought;
    } else 
    if (lNetworkEvents & (FD_READ | FD_ACCEPT | FD_CLOSE)) {
        int r, error;

        char data[64];
        WSASetLastError (0);
        r = recv (h, data, sizeof (data), MSG_PEEK);
        error = WSAGetLastError ();
        WSASetLastError (0);

        if ((r > 0) || (error == WSAENOTCONN))
            happened |= (POLLIN | POLLRDNORM) & sought;
        else 
        if ((r == 0) || (error == WSAESHUTDOWN) || (error == WSAECONNRESET) || (error == WSAECONNABORTED) || (error == WSAENETRESET))
            happened |= POLLHUP;
        else
            happened |= POLLERR;
    }

    if (lNetworkEvents & (FD_WRITE | FD_CONNECT))
        happened |= (POLLOUT | POLLWRNORM | POLLWRBAND) & sought;

    if (lNetworkEvents & FD_OOB)
        happened |= (POLLPRI | POLLRDBAND) & sought;

    return happened;
}

int poll(struct pollfd *pfd, unsigned long nfd, int timeout) {
    struct timeval tv;

    HANDLE hEvent;
    WSANETWORKEVENTS ev;
    HANDLE h, handle_array[FD_SETSIZE + 2];
    DWORD ret, wait_timeout, nhandles;
    fd_set rfds, wfds, xfds;
    BOOL poll_again;
    MSG msg;
    int rc = 0;
    unsigned long i;

    hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

restart:
    handle_array[0] = hEvent;
    nhandles = 1;
    FD_ZERO (&rfds);
    FD_ZERO (&wfds);
    FD_ZERO (&xfds);

    for (i = 0; i < nfd; i++) {
        int sought = pfd[i].events;
        pfd[i].revents = 0;
        if (pfd[i].fd < 0)
            continue;
        if (!(sought & (POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM | POLLWRBAND | POLLPRI | POLLRDBAND)))
            continue;

        h = HandleFromFd (pfd[i].fd);
        assert (h != NULL && h != INVALID_HANDLE_VALUE);
        if (IsSocketHandle (h)) {
            int requested = FD_CLOSE;

            if (sought & (POLLIN | POLLRDNORM)) {
                requested |= FD_READ | FD_ACCEPT;
                FD_SET ((SOCKET) h, &rfds);
            }
            if (sought & (POLLOUT | POLLWRNORM | POLLWRBAND)) {
                requested |= FD_WRITE | FD_CONNECT;
                FD_SET ((SOCKET) h, &wfds);
            }
            if (sought & (POLLPRI | POLLRDBAND)) {
                requested |= FD_OOB;
                FD_SET ((SOCKET) h, &xfds);
            }

            if (requested)
                WSAEventSelect ((SOCKET) h, hEvent, requested);
        } else {
            pfd[i].revents = windows_compute_revents (h, &sought);
            if (sought)
                handle_array[nhandles++] = h;
            if (pfd[i].revents)
                timeout = 0;
        }
    }
    tv.tv_sec = tv.tv_usec = 0;
    if (select (0, &rfds, &wfds, &xfds, &tv) > 0) {
        poll_again = FALSE;
        wait_timeout = 0;
    } else {
        poll_again = TRUE;
        if (timeout == -1)
            wait_timeout = INFINITE;
        else
        wait_timeout = timeout;
    }

    for (;;) {
        ret = MsgWaitForMultipleObjects (nhandles, handle_array, FALSE, wait_timeout, QS_ALLINPUT);

        if (ret == WAIT_OBJECT_0 + nhandles) {
            BOOL bRet;
            while ((bRet = PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) != 0) {
                if (msg.message == WM_QUIT) {
                    raise(SIGTERM);
                } else {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
        } else
            break;
    }

    if (poll_again)
        select (0, &rfds, &wfds, &xfds, &tv);

    handle_array[nhandles] = NULL;
    nhandles = 1;
    for (i = 0; i < nfd; i++) {
        int happened;

        if (pfd[i].fd < 0)
        continue;
        if (!(pfd[i].events & (POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM | POLLWRBAND)))
            continue;

        h = (HANDLE) HandleFromFd (pfd[i].fd);
        if (h != handle_array[nhandles]) {
            WSAEnumNetworkEvents ((SOCKET) h, NULL, &ev);
            WSAEventSelect ((SOCKET) h, 0, 0);

            set_blocking_mode(pfd[i].fd, 1);

            if (FD_ISSET ((SOCKET) h, &rfds) && !(ev.lNetworkEvents & (FD_READ | FD_ACCEPT)))
                ev.lNetworkEvents |= FD_READ | FD_ACCEPT;
            if (FD_ISSET ((SOCKET) h, &wfds))
                ev.lNetworkEvents |= FD_WRITE | FD_CONNECT;
            if (FD_ISSET ((SOCKET) h, &xfds))
                ev.lNetworkEvents |= FD_OOB;

            happened = windows_compute_revents_socket ((SOCKET) h, pfd[i].events, ev.lNetworkEvents);
        } else {
            int sought = pfd[i].events;
            happened = windows_compute_revents (h, &sought);
            nhandles++;
        }

        if ((pfd[i].revents |= happened) != 0)
            rc++;
    }

    if (!rc && timeout == -1) {
        SleepEx (1, TRUE);
        goto restart;
    }
    CloseHandle(hEvent);
    return rc;
}
