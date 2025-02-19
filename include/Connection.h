#pragma once
#include <ctime>
#include <mysql.h>
#include <string>
#include <iostream>
/*
实现MySQL数据库的增删改查操作
*/

class Connection
{
public:
    // 初始化数据库连接
    Connection();

    // 释放数据库连接资源
    ~Connection();

    // 连接数据库
    bool connect(std::string ip,
                 unsigned short port,
                 std::string user,
                 std::string password,
                 std::string dbname);

    // 更新操作 insert、delete、update
    bool update(std::string sql);

    //查询操作 select
    MYSQL_RES* query(std::string sql);

    //刷新一下连接的起始空闲时间点
    void refreshAliveTime() { _aliveTime = clock(); }

    //返回存活的时间
    int getAliveTime() { return (clock() - _aliveTime)/CLOCKS_PER_SEC ; };
private:
    MYSQL *_conn; // 表示和MySQL Server的一条连接
    clock_t _aliveTime; //记录进入空闲状态后的起始存活时间
};
