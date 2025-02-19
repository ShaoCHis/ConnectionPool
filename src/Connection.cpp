#include "public.h"
#include "pch.h"
#include "Connection.h"

// 初始化数据库连接
Connection::Connection()
{
    _conn = mysql_init(nullptr);
}

// 释放数据库连接资源
Connection::~Connection()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
bool Connection::connect(std::string ip,
                         unsigned short port,
                         std::string user,
                         std::string password,
                         std::string dbname)
{
    // c_str 将str对象转换为 const char * 类型
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), password.c_str(),
                                  dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

// 更新操作 insert、delete、update
bool Connection::update(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG("更新失败:" + sql);
        return false;
    }
    return mysql_use_result(_conn);
}

// 查询操作 select
MYSQL_RES *Connection::query(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG("查询失败:" + sql);
        return nullptr;
    }
    return mysql_use_result(_conn);
}
