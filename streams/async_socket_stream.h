#ifndef __YK_STREAMS_ASYNC_SOCKET_STREAM_H__
#define __YK_STREAMS_ASYNC_SOCKET_STREAM_H__

#include "yk/thread.h"
#include "yk/scheduler.h"
#include "yk/worker.h"
#include "yk/fiber.h"
#include "yk/iomanager.h"
#include "yk/timer.h"
#include "yk/macro.h"
#include "yk/socket_stream.h"
#include <list>
#include <unordered_map>
#include <boost/any.hpp>
#include <thread>
namespace yk {


class AsyncSocketStream : public SocketStream
                         , public std::enable_shared_from_this<AsyncSocketStream> {
public:
    typedef std::shared_ptr<AsyncSocketStream> ptr;
    typedef yk::RWMutex RWMutexType;
    typedef std::function<bool(AsyncSocketStream::ptr)> connect_callback;
    typedef std::function<void(AsyncSocketStream::ptr)> disconnect_callback;

    AsyncSocketStream(Socket::ptr sock, bool owner = true);

    virtual bool start();

    virtual void close() override;

public:
    enum Error {
        OK = 0,         // 正常
        TIMEOUT = -1,   // 超时
        IO_ERROR = -2,  // IO 错误
        NOT_CONNECT = -3,  // 未连接
    };

protected:
    struct SendCtx {
    public:
        typedef std::shared_ptr<SendCtx> ptr;
        virtual ~SendCtx() {}

        // 执行发送操作
        virtual bool doSend(AsyncSocketStream::ptr stream) = 0;
    };

    // 接收上下文结构体，用于异步接收数据
    struct Ctx : public SendCtx {
    public:
        typedef std::shared_ptr<Ctx> ptr;
        virtual ~Ctx() {}

        // 构造函数
        Ctx();

        uint32_t sn;            // 序列号
        uint32_t timeout;       // 超时时间
        uint32_t result;        // 操作结果
        bool timed;             // 是否超时

        Scheduler* scheduler;   // 调度器指针
        Fiber::ptr fiber;       // 协程指针
        Timer::ptr timer;       // 定时器指针

        // 执行响应操作
        virtual void doRsp();
    };

public:
    // 设置工作线程管理器
    void setWorker(yk::IOManager* v) { m_worker = v;}
    yk::IOManager* getWorker() const { return m_worker;}

    // 设置 IO 管理器
    void setIOManager(yk::IOManager* v) { m_iomanager = v;}
    yk::IOManager* getIOManager() const { return m_iomanager;}

    // 获取是否自动连接标志位
    bool isAutoConnect() const { return m_autoConnect;}
    void setAutoConnect(bool v) { m_autoConnect = v;}

    // 获取连接回调函数和断开连接回调函数
    connect_callback getConnectCb() const { return m_connectCb;}
    disconnect_callback getDisconnectCb() const { return m_disconnectCb;}
    void setConnectCb(connect_callback v) { m_connectCb = v;}
    void setDisconnectCb(disconnect_callback v) { m_disconnectCb = v;}

    // 设置数据，支持任意类型
    template<class T>
    void setData(const T& v) { m_data = v;}

    // 获取数据，支持任意类型
    template<class T>
    T getData() const {
        try {
            return boost::any_cast<T>(m_data);
        } catch (...) {
        }
        return T();
    }

protected:
    // 执行读操作
    virtual void doRead();

    // 执行写操作
    virtual void doWrite();

    // 启动读操作
    virtual void startRead();

    // 启动写操作
    virtual void startWrite();

    // 超时回调函数
    virtual void onTimeOut(Ctx::ptr ctx);

    // 执行接收操作，由子类实现
    virtual Ctx::ptr doRecv() = 0;

    // 根据序列号获取上下文
    Ctx::ptr getCtx(uint32_t sn);

    // 根据序列号获取并删除上下文
    Ctx::ptr getAndDelCtx(uint32_t sn);

    // 根据序列号获取上下文并转换为指定类型
    template<class T>
    std::shared_ptr<T> getCtxAs(uint32_t sn) {
        auto ctx = getCtx(sn);
        if(ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    // 根据序列号获取并删除上下文并转换为指定类型
    template<class T>
    std::shared_ptr<T> getAndDelCtxAs(uint32_t sn) {
        auto ctx = getAndDelCtx(sn);
        if(ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    // 添加上下文
    bool addCtx(Ctx::ptr ctx);

    // 将发送上下文加入队列
    bool enqueue(SendCtx::ptr ctx);

    // 内部关闭连接
    bool innerClose();

    // 等待协程
    bool waitFiber();

protected:
    yk::FiberSemaphore m_sem;               // 协程信号量
    yk::FiberSemaphore m_waitSem;           // 等待协程信号量
    RWMutexType m_queueMutex;                   // 发送队列互斥锁
    std::list<SendCtx::ptr> m_queue;            // 发送队列
    RWMutexType m_mutex;                        // 上下文互斥锁
    std::unordered_map<uint32_t, Ctx::ptr> m_ctxs; // 序列号与上下文的映射表

    uint32_t m_sn;                              // 序列号
    bool m_autoConnect;                         // 是否自动连接
    yk::Timer::ptr m_timer;                  // 定时器
    yk::IOManager* m_iomanager;              // IO 管理器
    yk::IOManager* m_worker;                 // 工作线程管理器

    connect_callback m_connectCb;               // 连接回调函数
    disconnect_callback m_disconnectCb;         // 断开连接回调函数

    boost::any m_data;                          // 数据

};

// 异步 Socket 流管理器类
class AsyncSocketStreamManager {
public:
    typedef yk::RWMutex RWMutexType;
    typedef AsyncSocketStream::connect_callback connect_callback;
    typedef AsyncSocketStream::disconnect_callback disconnect_callback;

    // 构造函数
    AsyncSocketStreamManager();

    // 析构函数
    virtual ~AsyncSocketStreamManager() {}

    // 添加异步 Socket 流
    void add(AsyncSocketStream::ptr stream);

    // 清空异步 Socket 流
    void clear();

    // 设置连接
    void setConnection(const std::vector<AsyncSocketStream::ptr>& streams);

    // 获取异步 Socket 流
    AsyncSocketStream::ptr get();

    // 获取指定类型的异步 Socket 流
    template<class T>
    std::shared_ptr<T> getAs() {
        auto rt = get();
        if(rt) {
            return std::dynamic_pointer_cast<T>(rt);
        }
        return nullptr;
    }

    // 获取连接回调函数和断开连接回调函数
    connect_callback getConnectCb() const { return m_connectCb;}
    disconnect_callback getDisconnectCb() const { return m_disconnectCb;}
    void setConnectCb(connect_callback v);
    void setDisconnectCb(disconnect_callback v);

private:
    RWMutexType m_mutex;                        // 互斥锁
    uint32_t m_size;                            // 大小
    std::atomic<uint32_t> m_idx;                             // 索引
    std::vector<AsyncSocketStream::ptr> m_datas; // 异步 Socket 流列表
    connect_callback m_connectCb;               // 连接回调函数
    disconnect_callback m_disconnectCb;         // 断开连接回调函数
};

}

#endif
