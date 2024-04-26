#ifndef __YK_SQL_H__
#define __YK_SQL_H__
#include<string>
#include<memory>
#include<mysql_driver.h>
#include<mysql_connection.h>
#include<cppconn/statement.h>
#include<cppconn/resultset.h>
#include<cppconn/prepared_statement.h>
#include"yk/config.h"
#include"yk/singleton.h"
#include"yk/thread.h"
#include <functional>
namespace yk{
class Conn
{
public:
    typedef std::shared_ptr<Conn> ptr;
	Conn(sql::Connection* conn,sql::Statement* stmt);
    //返回最近的操作时间
    uint64_t getAliveTime(){return m_aliveTime;}
    //检测连接是否有效
    bool isValid(){return m_conn->isValid();}
	// 增加数据
	void addData(const std::string& sql);
	// 删
	void deleteData(const std::string& sql);

	// 改
	void updataData(const std::string& sql);

	// 查
    template<class T>
    using DataProcessor = std::function<void(sql::ResultSet*, T&)>;
	
    template<class T>
    void processQueryResult(std::vector<T>& datas, const std::string& sql, DataProcessor<T> processor) {
        m_aliveTime = GetCurrentMS();
        try {
            std::shared_ptr<sql::PreparedStatement> ps(m_conn->prepareStatement(sql)) ;
            std::shared_ptr<sql::ResultSet> res (ps->executeQuery()) ;
            while (res->next()) {
                T t;
                // 调用传入的回调函数处理查询结果
                processor(res.get(), t);
                datas.push_back(t);
            }
        } catch (sql::SQLException &e) {
            std::cerr << "SQLException: " << e.what() << std::endl;
        }

    }
    Conn& operator = (const Conn& s) = default; 
	~Conn();
private:
    uint64_t m_aliveTime = 0;
	sql::Connection* m_conn=nullptr;
	sql::Statement* m_stmt=nullptr;
};


}
#endif