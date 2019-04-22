#ifndef SIMPLE_TCP_H
#define SIMPLE_TCP_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct simple_tcp simple_tcp;
typedef struct simple_tcp_msg simple_tcp_msg;

enum simple_tcp_errno {
    ERROR_CREATE_SOCKET = 1,
    ERROR_BIND,
    ERROR_CONNECT,
    ERROR_LISTEN,
    ERROR_ACCEPT,
    ERROR_READ,
    ERROR_WRITE,
    ERROR_CLOSE,
    COMPLETED_SEND,
    COMPLETED_RECV
};

struct simple_tcp_msg {
    size_t len;
    char data[];
};

struct simple_tcp {
    int fd;
    struct sockaddr_in src_addr;
    struct sockaddr_in dest_addr;
};

extern simple_tcp * simple_tcp_new();
extern void simple_tcp_free(simple_tcp * tcp);
extern int simple_tcp_connect(simple_tcp * tcp, const char * addr, int port);
extern int simple_tcp_bind(simple_tcp * tcp, const char * addr, int port);
extern int simple_tcp_listen(simple_tcp * tcp, int backlog);
extern int simple_tcp_accept(simple_tcp * listen_tcp, simple_tcp * child_tcp);
extern int simple_tcp_send(const simple_tcp * tcp, void * msg, size_t len);
extern int simple_tcp_send_nonblock(const simple_tcp * tcp, void * msg, size_t len, size_t * sended);
extern int simple_tcp_recv(const simple_tcp * tcp, simple_tcp_msg ** msg);
extern int simple_tcp_recv_nonblock(const simple_tcp * tcp, simple_tcp_msg ** msg, size_t * recved);
extern int simple_tcp_close(simple_tcp * tcp);

#endif /* end of include guard: SIMPLE_TCP_H */
