#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace yk {

// 定义静态日志器对象
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

// 线程局部变量，用于存储当前线程的 Scheduler 对象
static thread_local Scheduler* t_scheduler = nullptr;
// 线程局部变量，用于存储当前线程的主协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) {
    // 确保线程数量大于0
    YK_ASSERT1(threads > 0);

    if(use_caller) {
        // 如果使用调用线程作为调度器线程，则减少线程数量，并设置当前线程的调度器对象
        yk::Fiber::GetThis();
        --threads;

        // 断言当前线程没有调度器对象
        YK_ASSERT1(GetThis() == nullptr);
        t_scheduler = this;

        // 创建根协程，绑定调度器的运行函数并设置为主协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        yk::Thread::SetName(m_name);

        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = yk::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

// 调度器析构函数
Scheduler::~Scheduler() {
    YK_ASSERT1(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    YK_ASSERT1(m_threads.empty());

    // 创建指定数量的线程，并启动调度器运行函数
    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
}

void Scheduler::stop() {
    // 设置自动停止标志
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        YK_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    if(m_rootThread != -1) {
        YK_ASSERT1(GetThis() == this);
    } else {
        YK_ASSERT1(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }

    if(m_rootFiber) {
        if(!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
}

// 设置当前线程的调度器对象
void Scheduler::setThis() {
    t_scheduler = this;
}

// 调度器运行逻辑
void Scheduler::run() {
    // 记录调度器运行信息到日志
    YK_LOG_DEBUG(g_logger) << m_name << " run";
    // 启用 Hook 机制，用于协程上下文切换时的额外操作
    set_hook_enable(true);
    // 设置当前线程的调度器对象为当前对象
    setThis();
    // 如果当前线程不是根线程，则将当前主协程设置为当前协程
    if(yk::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }
    
    // 创建一个空闲状态的协程对象
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    // 创建一个协程对象，用于保存需要执行的回调函数
    Fiber::ptr cb_fiber;
    FiberAndThread ft;
    // 循环执行调度逻辑
    while(true) {
        // 重置协程和线程状态信息
        ft.reset();
        // 初始化唤醒标志和活跃状态标志
        bool tickle_me = false;
        bool is_active = false;
        {
            // 对线程安全的协程队列进行加锁保护
            MutexType::Lock lock(m_mutex);
            // 遍历协程队列，尝试获取要执行的协程
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                // 如果协程指定了线程，并且不是当前线程，则跳过当前协程
                if(it->thread != -1 && it->thread != yk::GetThreadId()) {
                    ++it;
                    // 标记需要唤醒调度器
                    tickle_me = true;
                    continue;
                }
                
                // 断言协程或回调函数不能为空
                YK_ASSERT1(it->fiber || it->cb);
                // 如果协程已经在执行状态，则继续下一个协程
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                
                // 获取到要执行的协程
                ft = *it;
                // 从协程队列中移除该协程
                m_fibers.erase(it++);
                // 增加活跃线程计数
                ++m_activeThreadCount;
                // 设置活跃状态标志
                is_active = true;
                break;
            }
            // 标记需要唤醒调度器
            tickle_me |= it != m_fibers.end();
        }

        // 唤醒调度器
        if(tickle_me) {
            tickle();
        }

        // 执行获取到的协程或回调函数
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            // 如果协程状态为就绪，则重新调度该协程
            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                // 如果协程状态不是终止或异常，将协程状态设置为挂起
                ft.fiber->m_state = Fiber::HOLD;
            }
            // 重置协程状态信息
            ft.reset();
        } else if(ft.cb) {
            // 如果存在回调函数，则执行回调函数
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            // 根据协程状态进行处理
            if(cb_fiber->getState() == Fiber::READY) {
                // 如果回调函数协程状态为就绪，则重新调度该协程
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                // 如果回调函数协程状态为异常或终止，则重置协程对象
                cb_fiber->reset(nullptr);
            } else {
                // 否则将协程状态设置为挂起
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            // 如果没有获取到协程或回调函数
            if(is_active) {
                // 如果当前是活跃状态，则减少活跃线程计数，并继续下一个循环
                --m_activeThreadCount;
                continue;
            }
            // 如果当前是空闲状态
            if(idle_fiber->getState() == Fiber::TERM) {
                // 如果空闲协程已经终止，则记录日志并退出循环
                YK_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            // 增加空闲线程计数
            ++m_idleThreadCount;
            // 切换到空闲协程执行
            idle_fiber->swapIn();
            // 减少空闲线程计数
            --m_idleThreadCount;
            // 如果空闲协程状态不是终止或异常，则将协程状态设置为挂起
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}


// 唤醒调度器
void Scheduler::tickle() {
    YK_LOG_INFO(g_logger) << "tickle";
}

// 判断调度器是否正在停止
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

// 空闲状态处理逻辑
void Scheduler::idle() {
    YK_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        yk::Fiber::YieldToHold();
    }
}

}
