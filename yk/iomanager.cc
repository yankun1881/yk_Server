#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <fcntl.h>
namespace yk{
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event){
    switch(event){
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            YK_ASSERT2(false,"getContext")
    }
}
void IOManager::FdContext::resetContext(EventContext& ctx){
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}
void IOManager::FdContext::triggerEvent(Event event){
    YK_ASSERT1(events & event);
    events = (Event)(events &  ~event);
    EventContext& ctx  = getContext(event);
    if(ctx.cb){
        ctx.scheduler->schedule(&ctx.cb);
    }else{  
        ctx.scheduler->schedule(&ctx.fiber);
    }   
    ctx.scheduler = nullptr;
}

IOManager::IOManager(size_t threads,bool use_caller , const std::string& name )
    :Scheduler(threads,use_caller,name){
    m_epfd = epoll_create(5000);
    YK_ASSERT1(m_epfd > 0);
    int rt = pipe(m_tickleFds);
    YK_ASSERT1(!rt);

    epoll_event event;
    memset(&event,0,sizeof(epoll_event));

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0],F_SETFL,O_NONBLOCK);
    YK_ASSERT1(!rt);
    
    rt = epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_tickleFds[0],&event);
    YK_ASSERT1(!rt);
    contextResize(32);
    start();

}
IOManager::~IOManager(){
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);
    for(size_t i = 0; i < m_fdContexts.size();++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size){
    m_fdContexts.resize(size);
    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i]  = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
} 

int IOManager::addEvent(int fd,Event event, std::function<void()> cb){
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd){
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }else{
        lock.unlock();
        RWMutexType::WriteLock lock(m_mutex);
        contextResize(fd*1.5);
        fd_ctx = m_fdContexts[fd];
    }
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    if(fd_ctx->events & event){
        YK_LOG_ERROR(g_logger) << "addEvent assert fd="<< fd
                                <<" event =" <<event
                                << " fd_ctx.event = " << fd_ctx->events;
    }
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        YK_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<", "
            <<op << "," << fd << "," <<epevent.events << "):"
            << rt << "  (" << errno << ") (" << strerror(errno) <<")";
        return  -1; 
    }
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    YK_ASSERT1(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb){
        event_ctx.cb.swap(cb);
    }else{
        event_ctx.fiber = Fiber::GetThis();
        YK_ASSERT1(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}


bool IOManager::delEvent(int fd,Event event){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    if(!(fd_ctx->events & event)){
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        YK_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<", "
            <<op << "," << fd << "," <<epevent.events << "):"
            << rt << "  (" << errno << ") (" << strerror(errno) <<")";
        return  false; 
    }

    --m_pendingEventCount;

    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);

    return true;

}

bool IOManager::cancelEvent(int fd,Event event){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    if(!(fd_ctx->events & event)){
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        YK_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<", "
            <<op << "," << fd << "," <<epevent.events << "):"
            << rt << "  (" << errno << ") (" << strerror(errno) <<")";
        return  false; 
    }
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;

    return true;
}

bool IOManager::cancelAll(int fd){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    if(!fd_ctx->events){
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        YK_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<", "
            <<op << "," << fd << "," <<epevent.events << "):"
            << rt << "  (" << errno << ") (" << strerror(errno) <<")";
        return  false; 
    }
    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;  
    }
    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    YK_ASSERT1(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager* >(Scheduler::GetThis());
}
void IOManager::tickle() {
    if(!hasIdleThreads()){
        return ;
    }
    int rt = write(m_tickleFds[1],"T",1);
    YK_ASSERT1(rt == 1);

}
bool IOManager::stopping(){
    uint64_t timeout = 0;

    return stopping(timeout);
}

bool IOManager::stopping(uint64_t& timeout){
    timeout = getNextTimer();
    return  timeout == ~0ull
            &&m_pendingEventCount == 0 
            &&Scheduler::stopping();
}

// 空闲状态处理逻辑
void IOManager::idle() {
    // 记录空闲状态信息到日志
    YK_LOG_DEBUG(g_logger) << "idle";
    // 定义最大事件数常量
    const uint64_t MAX_EVNETS = 256;
    // 创建 epoll_event 数组用于存放事件
    epoll_event* events = new epoll_event[MAX_EVNETS]();
    // 创建 shared_ptr 用于释放内存
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    // 循环执行空闲状态逻辑
    while(true) {
        // 获取下一个定时器的超时时间
        uint64_t next_timeout = 0;
        // 如果当前线程即将停止，则退出循环
        if(YK_UNLIKELY(stopping(next_timeout))) {
            YK_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        int rt = 0;
        // 执行 epoll_wait 等待事件
        do {
            static const int MAX_TIMEOUT = 3000;
            // 如果下一个超时时间不为无穷大，则限制在最大超时时间内
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
            // 如果返回值小于0且错误为 EINTR，则继续等待
            if(rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        // 定义存储定时器触发回调函数的容器
        std::vector<std::function<void()> > cbs;
        // 遍历定时器，查找已经超时的定时器并触发回调
        listExpiredCb(cbs);
        // 如果定时器回调函数不为空，则调度执行
        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        // 遍历 epoll_wait 返回的事件
        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            // 如果事件是由 tickle 触发的，则清空 tickle 管道
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            // 获取事件对应的上下文信息
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            // 上锁保护上下文信息
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            // 如果事件是错误或挂起事件，则转换为读写事件
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            // 定义真正触发的事件类型
            int real_events = NONE;
            // 如果有读事件，则添加到触发事件中
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            // 如果有写事件，则添加到触发事件中
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            // 如果事件无效，则继续下一个事件
            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            // 更新剩余的事件类型
            int left_events = (fd_ctx->events & ~real_events);
            // 根据剩余事件类型选择 EPOLL_CTL_MOD 或 EPOLL_CTL_DEL 操作
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            // 执行 epoll_ctl 更新事件
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            // 如果操作失败，则记录错误日志
            if(rt2) {
                YK_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
            // 触发读事件
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            // 触发写事件
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        // 切出当前协程
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}


void IOManager::onTimerInsertedAtFront(){
    tickle();

}

}