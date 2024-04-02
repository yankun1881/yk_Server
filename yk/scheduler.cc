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
    YK_LOG_DEBUG(g_logger) << m_name << " run";
    set_hook_enable(true);
    setThis();
    if(yk::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }
   // 创建一个空闲状态的协程对象
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                if(it->thread != -1 && it->thread != yk::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                YK_ASSERT1(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }

        if(tickle_me) {
            tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if(ft.cb) {
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                YK_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
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
