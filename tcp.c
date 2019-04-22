#include "tcp.h"

simple_tcp * simple_tcp_new() {
    simple_tcp * tcp = malloc(sizeof(simple_tcp));
    tcp->fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&tcp->src_addr, 0, sizeof(struct sockaddr_in));
    memset(&tcp->dest_addr, 0, sizeof(struct sockaddr_in));
    return tcp;
}

void simple_tcp_free(simple_tcp * tcp) {
    free(tcp);
}

int simple_tcp_bind(simple_tcp * tcp, const char * addr, int port) {
    struct sockaddr_in * _addr = &tcp->src_addr;
    _addr->sin_family = AF_INET;
    _addr->sin_port = htons(port);
    _addr->sin_addr.s_addr = inet_addr(addr);
    int retcode = bind(tcp->fd, (struct sockaddr *)_addr, sizeof(struct sockaddr_in));
    if (retcode) {
        return ERROR_BIND;
    }
    return 0;
}

int simple_tcp_listen(simple_tcp * tcp, int backlog) {
    int retcode = listen(tcp->fd, backlog);
    if (retcode) {
        return ERROR_LISTEN;
    }
    return 0;
}

int simple_tcp_connect(simple_tcp * tcp, const char * addr, int port) {
    struct sockaddr_in * _addr = &tcp->dest_addr;
    _addr->sin_family = AF_INET;
    _addr->sin_port = htons(port);
    _addr->sin_addr.s_addr = inet_addr(addr);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int retcode = connect(fd, (struct sockaddr *)_addr, sizeof(struct sockaddr_in));
    if (retcode) {
        return ERROR_CONNECT;
    }
    getsockname(fd, (struct sockaddr *)&tcp->src_addr, NULL);
    ioctl(fd, FIONBIO, 1);
    tcp->fd = fd;
    return 0;
}

int simple_tcp_accept(simple_tcp * listen_tcp, simple_tcp * child_tcp) {
    child_tcp->src_addr = listen_tcp->src_addr;
    int fd;
    socklen_t cli_len = sizeof(struct sockaddr_in);
    again:
    fd = accept(listen_tcp->fd, (struct sockaddr *)&child_tcp->dest_addr, &cli_len);
    if (fd < 0) {
        if (errno == ECONNABORTED || errno == EINTR) {
            goto again;
        }
        else {
            return ERROR_ACCEPT;
        }
    }
    child_tcp->fd = fd;
    ioctl(fd, FIONBIO, 1);
    return 0;
}

static int write_all(int fd, const void * src, size_t len) {
    ssize_t count = 0, sended;
    while (count < len) {
        again:
        sended = write(fd, src + count, len - count);
        if (sended < 0) {
            if (errno == EINTR) {
                goto again;
            }
            else {
                return ERROR_WRITE;
            }
        }
        count += sended;
    }
    return 0;
}

static int read_all(int fd, void * dest, size_t len) {
    ssize_t count = 0, recved;
    while (count < len) {
        again:
        recved = read(fd, dest + count, len - count);
        if (recved < 0) {
            if (errno == EINTR) {
                goto again;
            }
            else {
                return ERROR_READ;
            }
        }
        count += recved;
    }
    return 0;
}

int simple_tcp_send(const simple_tcp * tcp, void * msg, size_t len) {
    uint64_t size = len;
    int retcode;
    retcode = write_all(tcp->fd, &size, sizeof(size));
    if (retcode) {
        return ERROR_WRITE;
    }
    retcode = write_all(tcp->fd, msg, len);
    if (retcode) {
        return ERROR_WRITE;
    }
    return 0;
}

int simple_tcp_send_nonblock(const simple_tcp * tcp, void * msg, size_t len, size_t * sended) {
    if (*sended == 0) {
        uint64_t size = len;
        int retcode = write_all(tcp->fd, &size, sizeof(uint64_t));
        if (retcode) {
            return ERROR_WRITE;
        }
    }
    if (*sended == len) {
        return COMPLETED_SEND;
    }
    else {
        ssize_t count;
        again:
        count = write(tcp->fd, msg + *sended, len - *sended);
        if (count < 0) {
            if (errno == EINTR) {
                goto again;
            }
            else {
                return ERROR_WRITE;
            }
        }
        *sended += count;
    }
    return 0;
}

int simple_tcp_recv(const simple_tcp * tcp, simple_tcp_msg ** msg) {
    uint64_t len;
    int retcode;
    retcode = read_all(tcp->fd, &len, sizeof(len));
    if (retcode) {
        return ERROR_READ;
    }
    *msg = malloc(sizeof(simple_tcp_msg) + len);
    (*msg)->len = len;
    retcode = read_all(tcp->fd, (*msg)->data, len);
    if (retcode) {
        free(*msg);
        return ERROR_READ;
    }
    return 0;
}

int simple_tcp_recv_nonblock(const simple_tcp * tcp, simple_tcp_msg ** msg, size_t * recved) {
    if (*recved == 0) {
        uint64_t len;
        if (read_all(tcp->fd, &len, sizeof(uint64_t)) < 0) {
            return ERROR_READ;
        }
        *msg = malloc(sizeof(simple_tcp_msg) + len);
    }
    if (*recved == (*msg)->len) {
        return COMPLETED_RECV;
    }
    else {
        ssize_t count;
        again:
        count = read(tcp->fd, (*msg)->data + *recved, (*msg)->len - *recved);
        if (count < 0) {
            if (errno == EINTR) {
                goto again;
            }
            else {
                return ERROR_READ;
            }
        }
        *recved += count;
    }
    return 0;
}

int simple_tcp_close(simple_tcp * tcp) {
    if (close(tcp->fd) < 0) {
        return ERROR_CLOSE;
    }
    else {
        return 0;
    }
}
