#ifndef __YK_SQL_H__
#define __YK_SQL_H__
#include<string>
#include<filesystem>
#include<fstream>
#include<iostream>

#include<mysql_driver.h>
#include<mysql_connection.h>
#include<cppconn/statement.h>
#include<cppconn/resultset.h>
#include<cppconn/prepared_statement.h>
#include"yk/config.h"
#include"yk/singleton.h"
namespace yk{
class SQL
{
friend class LexicalCast<std::string, SQL>;
friend class LexicalCast<SQL, std::string>;
public:

	SQL();
    //连接数据库
    bool start();

    sql::Connection* getConnection(){return conn;}
	// 增加数据
	void addData(const std::string& sql);
	// 删
	void deleteData(const std::string& sql);

	// 改
	void updataData(const std::string& sql);

	// 查
	template<class T>
    void processQueryResult( std::vector<T>& datas,const std::string& sql)
    {
        if (!status) return datas;
        try {
            sql::PreparedStatement* ps = conn->prepareStatement(sql);
            sql::ResultSet* res = ps->executeQuery();
            while (res->next())
            {
                T t;
                getdata(res,t);
                datas.push_back(t);
            }    
            delete ps;
            delete res;
        }catch (sql::SQLException &e) {
            std::cerr << "SQLException: " << e.what() << std::endl;
        }
        return datas;
    }
    SQL& operator = (const SQL& s) = default; 
	~SQL();
private:
	bool status = false;
	std::string dress;
	std::string user;
	std::string passward;
	std::string database;

	sql::Connection* conn=nullptr;
	sql::mysql::MySQL_Driver* driver=nullptr;
	sql::Statement* stmt=nullptr;
};


  
template<>
class LexicalCast<std::string, SQL>{

public:
    SQL operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        SQL p;
        if(!node["dress"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取dress失败";
            return p;
        }
        if(!node["user"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取user失败";
            return p;
        }
        if(!node["passward"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取passward失败";
            return p;
        }
        if(!node["database"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取database失败";
            return p;
        }
        p.dress = node["dress"].as<std::string>();
        p.user = node["user"].as<std::string>();
        p.passward = node["passward"].as<std::string>();
        p.database = node["database"].as<std::string>();
        return p;        
    }
};

template<>
class LexicalCast<SQL, std::string>{
public:
    std::string operator() (const SQL & p){
        YAML::Node node;
        node["dress"] = p.dress;
        node["user"] = p.user;
        node["passward"] = p.passward;
        node["database"] = p.database;
        std::stringstream ss;
        ss << node;
        return ss.str();    
    }
};
typedef yk::Singleton<SQL> SQLMgr;
}
#endif