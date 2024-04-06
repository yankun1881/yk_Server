#include "../yk/log.cc"
#include "../yk/config.cc"
#include "../yk/util.h"
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include <list>

#include <iostream>
/*
yk::ConfigVar<int>::ptr g_int_value_config =
    yk::Config::Lookup("system.prot",(int) 8080,"system port");  // 第一个参数定位yml文件中的位置  注意同名直接报错

yk::ConfigVar<float>::ptr g_float_value_config =
    yk::Config::Lookup("system.value",(float)10.2f,"system value");  //

yk::ConfigVar<std::vector<int>>::ptr g_int_vector_value_config =
    yk::Config::Lookup("system.int_vec",std::vector<int>{2,3},"system value");  //

yk::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    yk::Config::Lookup("system.list",std::list<int>{2,3},"system value");  //


void print_yaml(const YAML::Node& node, int level){
    if(node.IsScalar()){
        YK_LOG_INFO(YK_LOG_ROOT())  << std::string(level*4,' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    }else if(node.IsNull()){
        YK_LOG_INFO(YK_LOG_ROOT())  << std::string(level*4,' ')
            << "NULL" << " - " << node.Type() << " - " << level;
    }else if(node.IsMap()){
        for(auto it = node.begin();
                it != node.end();++it){
            YK_LOG_INFO(YK_LOG_ROOT())  << std::string(level*4,' ')
            << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second,level+1);
        }
    }else if(node.IsSequence()){
        for(size_t i = 0; i < node.size();++i){
            YK_LOG_INFO(YK_LOG_ROOT())  << std::string(level*4,' ')
            << i << " - "  << node[i].Type() << " - " << level;
            print_yaml(node[i],level+1);
        }
    }
}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/log.yml");
    print_yaml(root,0);
    YK_LOG_INFO(YK_LOG_ROOT()) << root;
    std::cout << "1";
}

void test_config(){
    YK_LOG_INFO(YK_LOG_ROOT()) << "befrom: " << g_int_value_config->getValue();   
    YK_LOG_INFO(YK_LOG_ROOT()) << "befrom: " << g_float_value_config->toString();   
    auto  v = g_int_list_value_config->getValue();
    for(auto& i : v)
    YK_LOG_INFO(YK_LOG_ROOT()) <<"int_vec :"<<i << " ";  

    YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/log.yml");
    yk::Config::LoadFromYaml(root);
    YK_LOG_INFO(YK_LOG_ROOT())<< "after: " << g_int_value_config->getValue();
    YK_LOG_INFO(YK_LOG_ROOT())<< "after: " << g_float_value_config->toString();
    v = g_int_list_value_config->getValue();
    for(auto& i : v)
    YK_LOG_INFO(YK_LOG_ROOT()) <<"after: int_vec :"<<i << " ";  
}
*/


//任意类配置
class Person{
public:
    std::string m_name;
    int m_age;
    bool m_sex;
    bool operator==(const Person& p)
    {
        return m_name == p.m_name;
    }
    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name << " "
          << "age = " << m_age  << " "
          <<"m_sex = " << m_sex << " "
          <<']';
          return ss.str();
    }
};

namespace yk{
    
template<>
class LexicalCast<std::string, Person>{
public:
    Person operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        Person p;
        if(!node["name"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取name失败";
            return p;
        }
        if(!node["age"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取age失败";
            return p;
        }
        if(!node["sex"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取sex失败";
            return p;
        }
        
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;        
    }
};

template<>
class LexicalCast<Person, std::string>{
public:
    std::string operator() (const Person & p){
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();    
    }
};

}




yk::ConfigVar<Person>::ptr g_Person_value_config =
    yk::Config::Lookup("class.person",Person(),"system person");  

yk::ConfigVar<std::map<std::string,Person>>::ptr g_map_Person_value_config =
    yk::Config::Lookup("class.map",std::map<std::string,Person>(),"system person");  


void test_class(){
    YK_LOG_INFO(YK_LOG_ROOT()) << "befor :" << g_Person_value_config->getValue().toString() << " - "<< g_Person_value_config->toString() ;

    auto m = g_map_Person_value_config->getValue();
    for(auto& x : m){
        YK_LOG_INFO(YK_LOG_ROOT()) << "name :" << x.first << std::endl;
        YK_LOG_INFO(YK_LOG_ROOT()) << "           "<< x.second.toString() << std::endl;
    }
    
    YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/test.yml");
    yk::Config::LoadFromYaml(root);
    
    YK_LOG_INFO(YK_LOG_ROOT()) << "after :" << g_Person_value_config->getValue().toString() << " - "<< g_Person_value_config->toString(); 
    
    auto m1 = g_map_Person_value_config->getValue();
    for(auto& x : m1){
        YK_LOG_INFO(YK_LOG_ROOT()) << "name :" << x.first << std::endl;
        YK_LOG_INFO(YK_LOG_ROOT()) << "           "<< x.second.toString() << std::endl;
    }

}

void test_log(){
    static yk::Logger::ptr system_log = YK_LOG_NAME("system");
    YK_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << yk::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/log.yml");
    yk::Config::LoadFromYaml(root);
    std::cout << "===============" << std::endl;
    std::cout << yk::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YK_LOG_INFO(system_log) << "hello system" << std::endl;

}


void testVisit(){
    yk::Config::Visit([](yk::ConfigVarBase::ptr var){
        
    std::cout << yk::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/log.yml");
    YK_LOG_INFO(YK_LOG_ROOT()) << "name = " << var->getName()
        <<" description: "<< var->getDescription()
        <<" value = " << var->toString();        
    });
}

void test_loadconf(){
    yk::Config::LoadFromConfDir("conf");
}

int main(int argc,char** argv)
{
    //YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/log.yml");
    //yk::Config::LoadFromYaml(root);
    //YK_LOG_INFO(YK_LOG_ROOT()) << g_float_value_config->getValue();
    
    //test_yaml();
    //test_config();
    //test_class();
    //testVisit();
    yk::EnvMgr::GetInstance()->init(argc,argv);
    test_loadconf();
    return 0;
}