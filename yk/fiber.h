#ifndef __YK_FIBER_H__
#define __YK_FIBER_H__

#include<memory>
#include<functional>
#include<ucontext.h>
#include "thread.h"

namespace yk{
class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber>{
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;
    //std::shared_ptr<Fiber> ptr;
    enum State{
        
        INIT,   // 初始化状态
        HOLD,   // 暂停状态
        EXEC,   // 执行中状态
        TERM,   // 结束状态
        READY,  // 可执行状态
        EXCEPT  // 异常状态
    };
    

    Fiber(std::function<void()> cb,size_t stacksize = 0,bool use_caller=false);
    ~Fiber();

    //重置协程函数，并重置状态
    //INIT，TERM
    void reset(std::function<void()> cb);

    // 切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();
    
    void call();
    void back();
    uint64_t getId(){return m_id;};
    State getState() const { return m_state;}
public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前执行点的协程
    static  Fiber::ptr GetThis();
    //协程切换到后台，并设置为Ready状态
    static  void YieldToReady();
    //协程切换到后台，并设置为Hold状态
    static  void YieldToHold();
    //总协程数
    static  uint64_t TotalFibers();
    
    static  void  MainFunc();

    static  void  CallMainFunc();
    //返回协程号
    static uint64_t GetFiberId();

private:
    Fiber();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;
    
    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb; 
};






}







#endif