#include "sql.h"
#include "yk/log.h"
namespace yk{
static yk::Logger::ptr g_logger = YK_LOG_ROOT();
SQL::SQL(){

}
bool SQL::start()
{
	
	// 进行连接
	try
	{
		// 进行连接->创建驱动实例
		driver = sql::mysql::get_driver_instance();
		// 进行连接->进行mysql的连接
		conn = driver->connect(this->dress, this->user, this->passward);
		// 创建用于查询更新的句柄
		stmt = conn->createStatement();
		std::string useDatabaseQuery = "use ";
		stmt->execute(useDatabaseQuery+database+";");
		// 进行数据库的进入了
		status = true;
	}
	catch (const std::exception& e)
	{
		YK_LOG_ERROR(g_logger) << "SQLException: " << e.what() ;
		status = false;
	}
	return status;
}

void SQL::addData(const std::string& sql)
{
	if (!status) {
		YK_LOG_ERROR(g_logger) << "add data error\n";
		return;
	}
	conn->setAutoCommit(false); // 禁用自动提交
	sql::PreparedStatement* Ps = nullptr;
	try {
		Ps = conn->prepareStatement(sql);
		Ps->executeUpdate();
		conn->commit(); // 提交事务
	}
	catch (sql::SQLException& e) {
		conn->rollback(); // 回滚事务
	}

	if (Ps) {
		delete Ps;
	}
}

void SQL::deleteData(const std::string& sql)
{
	if (!status) {
		YK_LOG_ERROR(g_logger) << "delete data error\n" << std::endl;
		return;
	}
	try
	{
		stmt->execute(sql);
	}
	catch (const std::exception& e)
	{
		YK_LOG_ERROR(g_logger) << e.what() << std::endl;
	}
}

void SQL::updataData(const std::string& sql)
{
	if (!status) {
		YK_LOG_ERROR(g_logger) << "updata data error\n" << std::endl;
		return;
	}
	sql::PreparedStatement* ps = nullptr;
	try
	{
		// 预处理语句
		ps = conn->prepareStatement(sql);
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


SQL::~SQL()
{
	if (stmt) {
		delete stmt;
		stmt = nullptr;
	}
	if (conn) {
		conn->close();
		delete conn;
		conn = nullptr;
	}
}



}