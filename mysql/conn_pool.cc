#include"conn_pool.h"

namespace yk{
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
ConnPool::~ConnPool(){
    while (!m_conns.empty()) {
        auto conn = m_conns.front();
        m_conns.pop_front();
        delete conn;
    }
    if(driver){
        delete driver;
    }
}
void ConnPool::init(){
    if(m_status){
        return ;
    }
    try{
        // 进行连接->创建驱动实例
		driver = sql::mysql::get_driver_instance();
    }catch (const std::exception& e)
	{
		YK_LOG_ERROR(g_logger) << "SQLException: " << e.what() ;
		m_status = false;
	}
    m_status = true;
}
Conn::ptr ConnPool::getConn(){
    MutexType::Lock lock(m_mutex);
    while (!m_conns.empty()) {
        if (!m_conns.front()->isValid()) {
            auto conn = m_conns.front();
            m_conns.pop_front();
            delete conn;
        }else{
            break;
        }
    }    
    if(m_conns.empty()) {
        addConn();
    }
    Conn::ptr connptr(m_conns.front(), [this](Conn* conn) {
        MutexType::Lock lock(m_mutex);
        m_conns.push_back(conn); // 回收数据库连接，此时它再次处于空闲状态
    });// 智能指针
    m_conns.pop_front();
    return connptr;
}
void ConnPool::addConn(){
    if(!m_status) return ;
    try{
        auto conn = driver->connect(this->dress, this->user, this->passward);
        auto stmt = conn->createStatement();
		std::string useDatabaseQuery = "use ";
		stmt->execute(useDatabaseQuery+database+";");    
        auto sql = new Conn(conn,stmt);    
        m_conns.push_back(sql);
    }catch (const std::exception& e)
	{
		YK_LOG_ERROR(g_logger) << "SQLException: " << e.what();
	}
}

void ConnPool::produceConn() {
    while (true) {  // 生产者线程不断生产连接，直到连接池达到最小值
        sleep(10);
        MutexType::Lock lock(m_mutex);
        while (m_conns.size() <= m_minSize) {
            YK_LOG_ERROR(g_logger) << "conn pool size : " << m_conns.size();
            addConn();
        }
    }
}

void ConnPool::recycleConn(){
    while(true){
        sleep(10);
        MutexType::Lock lock(m_mutex);
        auto it = m_conns.begin();
        while (it != m_conns.end()) {
            auto& conn = *it;
            if (m_conns.size() <= m_minSize) {
                break; // 连接池中的连接数量已经达到最小值，不需要删除更多连接
            }
            if (GetCurrentMS() - conn->getAliveTime() > m_timeout) {
                it = m_conns.erase(it); // 删除超时的连接对象，并更新迭代器
                delete conn; // 释放连接对象的内存
            } else {
                ++it;
            }
        }
    }
}




}