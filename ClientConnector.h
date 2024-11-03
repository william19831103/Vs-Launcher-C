#pragma once
#include <string>


// 客户端信息单例类
class ClientInfo 
{
    public:
	static ClientInfo* instance()
	{
		static ClientInfo instance;
		return &instance;
	}
    std::string ip;
    std::string port;
    std::string name;
    std::string notice;
    bool isConnected;

};

#define sClientInfo ClientInfo::instance()

