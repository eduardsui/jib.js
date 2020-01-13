#ifndef POLL_H
#define POLL_H

#define POLLIN          0x01
#define POLLPRI         0x02
#define POLLOUT         0x04
#define POLLERR         0x08
#define POLLHUP         0x10
#define POLLNVAL        0x20

struct pollfd {
    int fd;
    short int events;
    short int revents;
};

int poll(struct pollfd *pfd, unsigned long nfd, int timeout);

#endif // POLL_H
