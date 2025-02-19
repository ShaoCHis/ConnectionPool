#pragma once
#include<string>
#include<queue>
#include "Connection.h"
#include "pch.h"
#include<mutex>
#include<atomic>
#include<condition_variable>
#include<thread>
#include<memory>
#include<functional>
/*
实现连接池功能
*/

class ConnectionPool{
public:
    //获取对象实例
    static ConnectionPool* getConnectionPool();
    //给外部提供接口，从连接池中获取一个可用的空闲连接
    std::shared_ptr<Connection> getConnection();
private:
    //构造函数私有化
    ConnectionPool();   //单例#1 构造函数私有化

    //从配置文件中加载配置项
    bool loadConfigFile();

    //运行在独立的线程中，专门负责生产新连接
    void produceConnectionTask();

    //扫描超过maxIdleTime时间的空闲连接，进行对于超时连接资源的回收
    void scannerConnectionTask();

    std::string _ip;    //mysql的ip地址
    unsigned short _port;   //mysql的登陆端口号 3306
    std::string _username;  //mysql的登陆用户名
    std::string _password;  //mysql的登陆密码
    std::string _dbname;    //mysql使用的数据库
    int _initSize;  //连接池的初始连接量
    int _maxSize;   //连接池的最大连接量
    int _maxIdleTime;   //连接池的最大空闲时间
    int _connectionTimeout; //连接池获取连接的超时时间

    std::queue<Connection*> _connectionQueue;   //存储mysql连接的队列，服务于多线程
    std::mutex _queueMutex; //维护连接队列的线程安全互斥锁
    std::atomic_int _connectionCnt; //用来记录连接所创建的connection连接的总数量
    std::condition_variable cv; //设置条件变量，用于连接生产线程和连接消费线程的通信
};

