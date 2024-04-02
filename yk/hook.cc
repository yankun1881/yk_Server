#include "hook.h"

#include "fiber.h"
#include "yk.h"
#include "iomanager.h"
#include "fdmanager.h"
#include <functional>
#include <vector>
#include <dlfcn.h>
#include <stdarg.h>

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
struct timer_info {
    int cancelled = 0;
};
namespace yk
{

static yk::ConfigVar<int>::ptr g_tcp_connect_timeout =
    yk::Config::Lookup("tcp.connect.timeout",5000,"tcp connect timeout");

static thread_local bool t_hook_enable = false;

// 定义一个宏，用于列举需要 hook 的函数
#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

// 初始化 hook 函数指针
void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return; // 如果已经初始化过，直接返回
    }
    
    // 使用宏展开来初始化函数指针
    #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
    #undef XX
}



static int s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
            YK_LOG_INFO(g_logger) << " tcp connect timeout changed from " << old_value <<" to " 
            << new_value;
            s_connect_timeout = new_value;
        });

    }
};

static _HookIniter s_hook_initer;


bool is_hook_enable(){
    return t_hook_enable;
}
void set_hook_enable(bool flag){
    t_hook_enable = flag;
}
    




template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {
    // 如果hook未启用，直接调用原始IO函数
    if(!yk::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }
    // 获取文件描述符上下文
    yk::FdCtx::ptr ctx = yk::FdMgr::GetInstance()->get(fd);
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 如果文件已关闭，则返回错误
    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    // 如果不是socket文件或用户主动设置了非阻塞，则直接调用原始IO函数
    if(!ctx->isSocket() || ctx->GetuserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    // 调用原始IO函数进行IO操作
    ssize_t n = fun(fd, std::forward<Args>(args)...);

    while(n == -1 && errno == EINTR) {  //重试
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 如果返回EAGAIN，说明操作会阻塞，则需要通过IO管理器进行异步IO操作
    if(n == -1 && errno == EAGAIN) {
        yk::IOManager* iom = yk::IOManager::GetThis();
        yk::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        // 如果设置了超时时间，则创建定时器
        if(to != (uint64_t)-1) {
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (yk::IOManager::Event)(event));//取消事件
            }, winfo);
        }

        // 将事件添加到IO管理器中
        int rt = iom->addEvent(fd, (yk::IOManager::Event)(event));
        if(rt) {
            // 添加事件失败，记录日志并取消定时器
            YK_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            // 切换到其他Fiber等待IO完成
            yk::Fiber::YieldToHold();
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) {  //已超时 
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry; //重新进行读取动作
        }
    }
    
    return n;
}



} // namespace yk

extern "C"{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX





unsigned int sleep(unsigned int seconds){
    if(!yk::t_hook_enable){
        return sleep_f(seconds);
    }
    yk::Fiber::ptr fiber = yk::Fiber::GetThis();
    yk::IOManager* iom = yk::IOManager::GetThis();
    //iom->addTimer(seconds * 1000, std::bind(&yk::IOManager::schedule<Fiber::ptr>, iom , fiber));
    iom->addTimer(seconds * 1000,[iom,fiber](){
        iom->schedule(fiber);
    });
    
    yk::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec){
    if(!yk::t_hook_enable){
        return usleep_f(usec);
    }
    yk::Fiber::ptr fiber = yk::Fiber::GetThis();
    yk::IOManager* iom = yk::IOManager::GetThis();
    iom->addTimer(usec * 1000,[iom,fiber](){
        iom->schedule(fiber);
    });
    yk::Fiber::YieldToHold();
    return 0;
}
 

int nanosleep (const struct timespec *req, struct timespec* rem){
    if(!yk::t_hook_enable){
        return nanosleep_f(req,rem);
    }
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec;
    yk::Fiber::ptr fiber = yk::Fiber::GetThis();
    yk::IOManager* iom = yk::IOManager::GetThis();
    iom->addTimer(timeout_ms,[iom,fiber](){
        iom->schedule(fiber);
    });
    return 0;
}


int socket(int domain,int type,int protocol){
    if(!yk::t_hook_enable){
        return socket_f(domain,type,protocol);
    }
    int fd = socket_f(domain,type,protocol);
    if(fd == -1){
        return fd;
    }
    yk::FdMgr::GetInstance()->get(fd,true);
    return fd;

}

// 带超时的connect函数实现
int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!yk::t_hook_enable) {  // 如果hook功能未启用，则直接调用原始的connect函数
        return connect_f(fd, addr, addrlen);
    }
    
    yk::FdCtx::ptr ctx = yk::FdMgr::GetInstance()->get(fd);  // 获取文件描述符上下文
    if(!ctx || ctx->isClose()) {  // 如果上下文不存在或者文件已关闭，则设置errno并返回-1
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()) {  // 如果不是套接字文件，则直接调用原始的connect函数
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->GetuserNonblock()) {  // 如果设置了用户非阻塞标志，则直接调用原始的connect函数
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);  // 调用原始的connect函数
    if(n == 0) {  // 连接成功，返回0
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {  // 如果连接不是正在进行中，则直接返回连接结果
        return n;
    }

    // 创建定时器
    yk::IOManager* iom = yk::IOManager::GetThis();
    yk::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1) {  // 如果设置了超时时间，则创建定时器
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
                auto t = winfo.lock();  // 使用弱引用获取定时器信息
                if(!t || t->cancelled) {  // 如果定时器信息不存在或已取消，则直接返回
                    return;
                }
                t->cancelled = ETIMEDOUT;  // 设置取消标志为超时
                iom->cancelEvent(fd, yk::IOManager::WRITE);  // 取消写事件
        }, winfo);
    }

    // 向IO线程注册写事件
    int rt = iom->addEvent(fd, yk::IOManager::WRITE);
    if(rt == 0) {  // 如果注册成功，则让出当前协程执行权，等待事件通知
        yk::Fiber::YieldToHold();  // 让出当前协程的执行权
        if(timer) {
            timer->cancel();  // 取消定时器
        }
        if(tinfo->cancelled) {  // 如果取消标志已被设置，则返回超时错误
            errno = tinfo->cancelled;
            return -1;
        }
    } else {  // 注册写事件失败
        if(timer) {
            timer->cancel();  // 取消定时器
        }
        YK_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";  // 打印日志
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {  // 获取socket选项错误信息
        return -1;
    }
    if(!error) {  // 如果没有错误，则连接成功，返回0
        return 0;
    } else {  // 如果有错误，则设置errno并返回-1
        errno = error;
        return -1;
    }
}

// 对外暴露的connect函数，使用默认的连接超时时间
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, yk::s_connect_timeout);  // 调用带超时的connect函数
}


int accept(int s, struct sockaddr *addr, socklen_t *addrlen){
    int fd = do_io(s,accept_f,"accept",yk::IOManager::READ, SO_RCVTIMEO,addr ,addrlen);
    if(fd >= 0){
        yk::FdMgr::GetInstance()->get(fd,true);
    }
    return fd;
}


ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", yk::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", yk::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", yk::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", yk::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", yk::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", yk::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", yk::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", yk::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", yk::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", yk::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!yk::t_hook_enable) {
        return close_f(fd);
    }

    yk::FdCtx::ptr ctx = yk::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = yk::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd); //取消文件描述符所对应的所有事件
        }
        yk::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd,int cmd,...){
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);  // 获取可变参数中的整型参数
                va_end(va);  // 结束可变参数的使用
                yk::FdCtx::ptr ctx = yk::FdMgr::GetInstance()->get(fd);  // 获取文件描述符上下文
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {  // 如果上下文不存在或者文件已关闭或者不是套接字文件，则调用原始的fcntl函数
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->SetuserNonblock(arg & O_NONBLOCK);  // 设置用户非阻塞标志
                if(ctx->GetuserNonblock()) {  // 如果系统非阻塞标志为真
                    arg |= O_NONBLOCK;  // 将O_NONBLOCK标志加入到参数中
                } else {
                    arg &= ~O_NONBLOCK;  // 否则将O_NONBLOCK标志从参数中移除
                }
                return fcntl_f(fd, cmd, arg);  // 调用原始的fcntl函数
            }
        break;
        case F_GETFL:
            {
                va_end(va);  // 结束可变参数的使用
                int arg = fcntl_f(fd, cmd);  // 调用原始的fcntl函数获取文件状态标志
                yk::FdCtx::ptr ctx = yk::FdMgr::GetInstance()->get(fd);  // 获取文件描述符上下文
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {  // 如果上下文不存在或者文件已关闭或者不是套接字文件，则直接返回获取到的标志
                    return arg;
                }
                if(ctx->GetuserNonblock()) {  // 如果用户非阻塞标志为真
                    return arg | O_NONBLOCK;  // 返回包含O_NONBLOCK标志的文件状态标志
                } else {
                    return arg & ~O_NONBLOCK;  // 返回移除O_NONBLOCK标志的文件状态标志
                }
            }
        break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}
int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);  // 获取可变参数中的指针参数
    va_end(va);  // 结束可变参数的使用

    if(request == FIONBIO) {  // 如果request是FIONBIO
        bool user_nonblock = !!*(int*)arg;  // 将指针参数转换成布尔值
        yk::FdCtx::ptr ctx = yk::FdMgr::GetInstance()->get(d);  // 获取文件描述符上下文
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {  // 如果上下文不存在或者文件已关闭或者不是套接字文件，则调用原始的ioctl函数
            return ioctl_f(d, request, arg);
        }
        ctx->SetuserNonblock(user_nonblock);  // 设置用户非阻塞标志
    }
    return ioctl_f(d, request, arg);  // 调用原始的ioctl函数
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);  // 调用原始的getsockopt函数
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!yk::t_hook_enable) { 
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {  // 如果选项级别是SOL_SOCKET
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {  // 如果选项名是SO_RCVTIMEO或者SO_SNDTIMEO
            yk::FdCtx::ptr ctx = yk::FdMgr::GetInstance()->get(sockfd);  // 获取文件描述符上下文
            if(ctx) {
                const timeval* v = (const timeval*)optval;  // 将选项值转换为timeval结构体
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);  // 设置超时时间
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);  // 调用原始的setsockopt函数
}


}






