#include "mysql/sql.h"
#include "yk/config.h"
#include "../chat/db/users/usersCtr.h"

yk::ConfigVar<yk::SQL>::ptr g_sql_value_config =
    yk::Config::Lookup("sql",yk::SQL(),"mysql");  


int main(int argc, char** argv){
    YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/mysql.yml");
    yk::Config::LoadFromYaml(root);
    auto sql = yk::SQLMgr::GetInstance();
    *sql = g_sql_value_config->getValue();
    std::cout << chat::UserController::NameByPassword("yk","123yankun");
    return 0;
}