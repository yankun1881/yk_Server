#ifndef __YK_TIMER_H__
#define __YK_TIMER_H__

#include<memory>

#include<vector>
#include<set>
#include "thread.h"

namespace yk{
class TimerManager;

class Timer: public std::enable_shared_from_this<Timer>{
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    Timer(uint64_t ms,std::function<void()> cb,
        bool recurring,TimerManager* manager);
    Timer(uint64_t next);

    // 取消定时器
    bool cancel();
    //刷新设置定时器的执行时间
    bool refresh();

    /**
     * @brief 重置定时器时间
     * @param[in] ms 定时器执行间隔时间(毫秒)
     * @param[in] from_now 是否从当前时间开始计算
     */
    bool reset(uint64_t ms,bool from_now);

private:
    bool m_recurring = false;       //是否循环定时器
    uint64_t  m_ms = 0;             //执行周期
    uint64_t  m_next = 0;           //精准的执行时间
    std::function<void()> m_cb;
    TimerManager* m_manager = nullptr;
private:
    //定时器比较函数
    struct Comparator{
        bool operator() (const Timer::ptr & lhs,const Timer::ptr& rhs) const;
    };
};

//定时器管理器
class TimerManager{
friend class Timer;
public:
    typedef RWMutex RWMutexType;
    
    TimerManager();

    virtual ~TimerManager();
    //添加定时器
    Timer::ptr addTimer(uint64_t ms,std::function<void()> cb
                        ,bool recurring = false);
    
    //添加条件定时器
    Timer::ptr addConditionTimer(uint64_t ms,std::function<void()> cb
                            ,std::weak_ptr<void> weak_cond
                            ,bool recurring = false);

    //到最近一个定时器执行的时间间隔(毫秒)
    uint64_t getNextTimer();
    
    //获取需要执行的定时器的回调函数列表
    void listExpiredCb(std::vector<std::function<void()>> &cbs);

    bool hasTime();

protected:

    //当有新的定时器插入到定时器的首部,执行该函数
    virtual void onTimerInsertedAtFront() = 0;

    
    void addTimer(Timer::ptr val,RWMutex::WriteLock& lock);
private:
    bool detectClockRollover(uint64_t now_ms); //服务器发送时间修改

private:
    RWMutexType m_mutex;
    std::set<Timer::ptr,Timer::Comparator> m_timers;
    uint64_t m_previouseTime;   //上一次执行的时间
    bool m_tickled = false;
};



}

#endif

