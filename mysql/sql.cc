#include "sql.h"
#include "yk/log.h"
namespace yk{
static yk::Logger::ptr g_logger = YK_LOG_ROOT();
Conn::Conn(sql::Connection* conn,sql::Statement *stmt)
	: m_conn(conn),m_stmt(stmt){
	m_aliveTime = GetCurrentMS();
}
void Conn::addData(const std::string& sql)
{
	m_aliveTime = GetCurrentMS();
	m_conn->setAutoCommit(false); // 禁用自动提交
	sql::PreparedStatement* Ps = nullptr;
	try {
		Ps = m_conn->prepareStatement(sql);
		Ps->executeUpdate();
		m_conn->commit(); // 提交事务
	}
	catch (sql::SQLException& e) {
		m_conn->rollback(); // 回滚事务
	}
	if (Ps) {
		delete Ps;
	}
}

void Conn::deleteData(const std::string& sql)
{
	m_aliveTime = GetCurrentMS();
	try
	{
		m_stmt->execute(sql);
	}
	catch (const std::exception& e)
	{
		YK_LOG_ERROR(g_logger) << e.what() << std::endl;
	}
}

void Conn::updataData(const std::string& sql)
{
	m_aliveTime = GetCurrentMS();
	sql::PreparedStatement* ps = nullptr;
	try
	{
		// 预处理语句
		ps = m_conn->prepareStatement(sql);
		ps->executeUpdate();
	}
	catch (const std::exception& e)
	{
		YK_LOG_ERROR(g_logger) << "sql updataData" << e.what() << std::endl;
	}
	if (ps) {
		delete ps;
	}

}


Conn::~Conn()
{
	if (m_stmt) {
		delete m_stmt;
		m_stmt = nullptr;
	}
	if (m_conn) {
		m_conn->close();
		delete m_conn;
		m_conn = nullptr;
	}
}



}