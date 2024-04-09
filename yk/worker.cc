#include "worker.h"
#include "config.h"
#include "util.h"
#include "macro.h"

namespace yk {

static yk::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_worker_config
    = yk::Config::Lookup("workers", std::map<std::string, std::map<std::string, std::string> >(), "worker config");


FiberSemaphore::FiberSemaphore(size_t initial_concurrency)
    :m_concurrency(initial_concurrency) {
}

FiberSemaphore::~FiberSemaphore() {
    YK_ASSERT1(m_waiters.empty());
}

bool FiberSemaphore::tryWait() {
    YK_ASSERT1(Scheduler::GetThis());
    {
        MutexType::Lock lock(m_mutex);
        if(m_concurrency > 0u) {
            --m_concurrency;
            return true;
        }
        return false;
    }
}

void FiberSemaphore::wait() {
    YK_ASSERT1(Scheduler::GetThis());
    {
        MutexType::Lock lock(m_mutex);
        if(m_concurrency > 0u) {
            --m_concurrency;
            return;
        }
        m_waiters.push_back(std::make_pair(Scheduler::GetThis(), Fiber::GetThis()));
    }
    Fiber::YieldToHold();
}

void FiberSemaphore::notify() {
    MutexType::Lock lock(m_mutex);
    if(!m_waiters.empty()) {
        auto next = m_waiters.front();
        m_waiters.pop_front();
        next.first->schedule(next.second);
    } else {
        ++m_concurrency;
    }
}


WorkerGroup::WorkerGroup(uint32_t batch_size, yk::Scheduler* s)
    :m_batchSize(batch_size)
    ,m_finish(false)
    ,m_scheduler(s)
    ,m_sem(batch_size) {
}

WorkerGroup::~WorkerGroup() {
    waitAll();
}

void WorkerGroup::schedule(std::function<void()> cb, int thread) {
    m_sem.wait();
    m_scheduler->schedule(std::bind(&WorkerGroup::doWork
                          ,shared_from_this(), cb), thread);
}

void WorkerGroup::doWork(std::function<void()> cb) {
    cb();
    m_sem.notify();
}

void WorkerGroup::waitAll() {
    if(!m_finish) {
        m_finish = true;
        for(uint32_t i = 0; i < m_batchSize; ++i) {
            m_sem.wait();
        }
    }
}

WorkerManager::WorkerManager()
    :m_stop(false) {
}

void WorkerManager::add(Scheduler::ptr s) {
    m_datas[s->getName()].push_back(s);
}

Scheduler::ptr WorkerManager::get(const std::string& name) {
    auto it = m_datas.find(name);
    if(it == m_datas.end()) {
        return nullptr;
    }
    if(it->second.size() == 1) {
        return it->second[0];
    }
    return it->second[rand() % it->second.size()];
}

IOManager::ptr WorkerManager::getAsIOManager(const std::string& name) {
    return std::dynamic_pointer_cast<IOManager>(get(name));
}

bool WorkerManager::init(const std::map<std::string, std::map<std::string, std::string> >& v) {
    for(auto& i : v) {
        std::string name = i.first;
        int32_t thread_num = yk::GetParamValue(i.second, "thread_num", 1);
        int32_t worker_num = yk::GetParamValue(i.second, "worker_num", 1);

        for(int32_t x = 0; x < worker_num; ++x) {
            Scheduler::ptr s;
            if(!x) {
                s = std::make_shared<IOManager>(thread_num, false, name);
            } else {
                s = std::make_shared<IOManager>(thread_num, false, name + "-" + std::to_string(x));
            }
            add(s);
        }
    }
    m_stop = m_datas.empty();
    return true;
}

bool WorkerManager::init() {
    auto workers = g_worker_config->getValue();
    return init(workers);
}

void WorkerManager::stop() {
    if(m_stop) {
        return;
    }
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->schedule([](){});
            n->stop();
        }
    }
    m_datas.clear();
    m_stop = true;
}

uint32_t WorkerManager::getCount() {
    return m_datas.size();
}

std::ostream& WorkerManager::dump(std::ostream& os) {
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->dump(os) << std::endl;
        }
    }
    return os;
}

}
