#ifndef __YK__SCHEDULER_H__
#define __YK__SCHEDULER_H__

#include <string>
#include <vector>
#include <list>
#include <memory>
#include "thread.h"
#include "fiber.h"

namespace yk {

/**
 * @brief 协程调度器
 */
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 构造函数
     * @param threads 协程调度器中的线程数量
     * @param use_caller 是否使用当前调用线程作为协程调度器的线程之一
     * @param name 协程调度器的名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    virtual ~Scheduler();

    /**
     * @brief 获取协程调度器的名称
     */ 
    const std::string& getName() const { return m_name; }

    /**
     * @brief 获取当前协程
     */   
    static Scheduler* GetThis();

    static Fiber* GetMainFiber();

    void start();

    void stop();

    bool hasIdleThreads() { return m_idleThreadCount > 0; }

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if(need_tickle) {
            tickle();
        }
    }
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }
protected:
    virtual void tickle(); // 唤醒等待中的线程
    virtual void run(); // 线程运行函数，实际执行协程调度的逻辑
    virtual bool stopping(); // 判断是否正在停止中
    virtual void setThis(); // 设置当前协程调度器
    virtual void idle(); // 空闲时执行的操作

private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
private:
    struct FiberAndThread {
        Fiber::ptr fiber; 
        std::function<void()> cb; // 回调函数
        
        int thread; // 线程ID

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f)
            , thread(thr) {
        }

        FiberAndThread(Fiber::ptr* f, int thr)
            : thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            : cb(f), thread(thr) {
        }

        FiberAndThread(std::function<void()>* f, int thr)
            : thread(thr) {
            cb.swap(*f);
        }

        FiberAndThread()
            : thread(-1) {
        }

        /**
         * @brief 重置数据
         */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    MutexType m_mutex; // 互斥锁
    std::vector<Thread::ptr> m_threads; // 线程数组
    std::list<FiberAndThread> m_fibers; // 协程及其所属线程列表
    Fiber::ptr m_rootFiber; // 根协程
    std::string m_name; // 协程调度器名称

protected:
    std::vector<int> m_threadIds; // 线程ID数组
    size_t m_threadCount = 0; // 线程数量
    std::atomic<size_t> m_activeThreadCount = {0}; // 活跃线程数
    std::atomic<size_t> m_idleThreadCount = {0}; // 空闲线程数
    bool m_stopping = true; // 是否正在停止中
    bool m_autoStop = false; // 是否自动停止
    int m_rootThread = 0; // 根线程ID
};

} // namespace yk

#endif
