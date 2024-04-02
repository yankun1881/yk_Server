#ifndef __YK__HOOK_H__
#define __YK__HOOK_H__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

namespace yk{
    bool is_hook_enable();
    void set_hook_enable(bool flag);
}

extern "C"{

// 定义 sleep 函数指针类型，用于使程序暂停执行指定的秒数
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

// 定义 usleep 函数指针类型，用于使程序暂停执行指定的微秒数
typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

// 定义 nanosleep 函数指针类型，用于使程序暂停执行指定的纳秒数
typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

// 定义 socket 函数指针类型，用于创建一个新的套接字
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

// 定义 connect 函数指针类型，用于与另一端建立连接
typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_fun connect_f;

// 定义 accept 函数指针类型，用于接受客户端的连接请求
typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

// 定义 read 函数指针类型，用于从文件描述符中读取数据
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

// 定义 readv 函数指针类型，用于在多个缓冲区中读取数据
typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

// 定义 recv 函数指针类型，用于从套接字接收数据
typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

// 定义 recvfrom 函数指针类型，用于从套接字接收数据并获取发送方信息
typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

// 定义 recvmsg 函数指针类型，用于从套接字接收数据和控制信息
typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

// 定义 write 函数指针类型，用于向文件描述符中写入数据
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

// 定义 writev 函数指针类型，用于将多个缓冲区的数据写入文件描述符
typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

// 定义 send 函数指针类型，用于向套接字发送数据
typedef ssize_t (*send_fun)(int s, const void *msg, size_t len, int flags);
extern send_fun send_f;

// 定义 sendto 函数指针类型，用于向套接字发送数据并指定目标地址
typedef ssize_t (*sendto_fun)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
extern sendto_fun sendto_f;

// 定义 sendmsg 函数指针类型，用于向套接字发送数据和控制信息
typedef ssize_t (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

// 定义 close 函数指针类型，用于关闭文件描述符或套接字
typedef int (*close_fun)(int fd);
extern close_fun close_f;

// 定义 fcntl 函数指针类型，用于对文件描述符进行控制操作
typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
extern fcntl_fun fcntl_f;

// 定义 ioctl 函数指针类型，用于对文件描述符进行设备控制
typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
extern ioctl_fun ioctl_f;

// 定义 getsockopt 函数指针类型，用于获取套接字选项值
typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

// 定义 setsockopt 函数指针类型，用于设置套接字选项值
typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

// 带超时参数的连接函数声明
extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);

}


#endif