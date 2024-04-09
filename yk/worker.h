#ifndef __YK_WORKER_H__   // 如果未定义 __YK_WORKER_H__ 则进入条件编译
#define __YK_WORKER_H__   // 定义 __YK_WORKER_H__ 避免重复包含

#include "thread.h"        // 包含线程相关的头文件
#include "singleton.h"     // 包含单例模式的头文件
#include "log.h"           // 包含日志记录的头文件
#include "iomanager.h"     // 包含IO管理器的头文件

namespace yk {            

// 定义协程信号量类
class FiberSemaphore {
public:
    typedef Spinlock MutexType;  // 使用自旋锁作为互斥量类型
    // 构造函数，初始并发量默认为 0
    FiberSemaphore(size_t initial_concurrency = 0);  
    ~FiberSemaphore();                               
    // 尝试非阻塞方式获取信号量
    bool tryWait(); 
    // 等待获取信号量      
    void wait();   
    // 通知等待者信号量可用       
    void notify();        
    // 获取当前并发量
    size_t getConcurrency() const { return m_concurrency; }
    // 重置并发量为 0  
    void reset() { m_concurrency = 0; }                       

private:
    MutexType m_mutex;                            // 互斥量
    std::list<std::pair<Scheduler*, Fiber::ptr>> m_waiters;  // 等待者列表，包含调度器和协程
    size_t m_concurrency;                         // 当前并发量
};

// 定义工作线程组类
class WorkerGroup : public std::enable_shared_from_this<WorkerGroup> {
public:
    typedef std::shared_ptr<WorkerGroup> ptr;  

    // 创建工作线程组的静态方法
    static WorkerGroup::ptr Create(uint32_t batch_size, yk::Scheduler* s = yk::Scheduler::GetThis()) {
        return std::make_shared<WorkerGroup>(batch_size, s);  // 使用工厂函数创建工作线程组
    }

    WorkerGroup(uint32_t batch_size, yk::Scheduler* s = yk::Scheduler::GetThis());  
    ~WorkerGroup();                                                                   
    // 调度任务到工作线程组
    void schedule(std::function<void()> cb, int thread = -1);  
    // 等待所有任务完成
    void waitAll();                                             

private:
    // 执行任务的私有方法
    void doWork(std::function<void()> cb);  

private:
    uint32_t m_batchSize;       // 批处理大小
    bool m_finish;              // 完成标志
    Scheduler* m_scheduler;     // 调度器指针
    FiberSemaphore m_sem;       // 纤程信号量
};

// 定义工作线程管理器类
class WorkerManager {
public:
    typedef std::shared_ptr<WorkerManager> ptr;  
    // 构造函数
    WorkerManager();                       
    // 添加调度器到管理器中
    void add(Scheduler::ptr s);            
    // 根据名称获取调度器
    Scheduler::ptr get(const std::string& name);  
    // 根据名称获取 IOManager 类型的调度器
    IOManager::ptr getAsIOManager(const std::string& name); 
    // 初始化工作线程管理器
    bool init();
    // 使用参数初始化         
    bool init(const std::map<std::string, std::map<std::string, std::string>>& v);  
    // 停止所有工作线程
    void stop();         
    // 判断是否停止
    bool isStoped() const { return m_stop; } 
    // 输出管理器状态到流中 
    std::ostream& dump(std::ostream& os);     
    // 获取调度器数量
    uint32_t getCount();  
private:
    std::map<std::string, std::vector<Scheduler::ptr>> m_datas;  // 调度器映射表
    bool m_stop;                    // 停止标志
};

typedef yk::Singleton<WorkerManager> WorkerMgr;  // 工作线程管理器单例类型

}  

#endif  
