#include "CommonConnectionPool.h"
#include "public.h"

// 线程安全的懒汉单例模式
ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool; // 对于静态变量，编译器会自动进行 lock和unlock
    return &pool;
}

bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("mysql.cnf", "r");
    if (pf == nullptr)
    {
        LOG("mysql.cnf file is not exist!");
        return false;
    }
    // 如果文件未到末尾
    while (!feof(pf))
    {
        char line[1024] = {0};
        fgets(line, 1024, pf);
        std::string str = line;
        int idx = str.find('=', 0);
        if (idx == -1) // 无效的配置项
        {
            continue;
        }
        // password = 123456\n
        int endIdx = str.find('\n', idx);
        std::string key = str.substr(0, idx);
        std::string value = str.substr(idx + 1, endIdx - idx - 1);
        if (key == "ip")
        {
            _ip = value;
        }
        else if (key == "port")
        {
            _port = atoi(value.c_str());
        }
        else if (key == "username")
        {
            _username = value;
        }
        else if (key == "password")
        {
            _password = value;
        }
        else if (key == "initSize")
        {
            _initSize = atoi(value.c_str());
        }
        else if (key == "maxSize")
        {
            _maxSize = atoi(value.c_str());
        }
        else if (key == "maxIdleTime")
        {
            _maxIdleTime = atoi(value.c_str());
        }
        else if (key == "connectionTimeout")
        {
            _connectionTimeout = atoi(value.c_str());
        }
        else if (key == "dbname")
        {
            _dbname = value;
        }
    }
    return true;
}

// 连接池的构造函数
ConnectionPool::ConnectionPool()
{
    // 加载配置项
    if (!loadConfigFile())
    {
        return;
    }

    /*
    创建初始数量的连接
    */
    for (int i = 0; i < _initSize; i++)
    {
        Connection *p = new Connection();
        if (p->connect(_ip, _port, _username, _password, _dbname))
        {
            p->refreshAliveTime(); // 刷新一下开始空闲的起始时间
            _connectionQueue.push(p);
            _connectionCnt++;
        }
    }

    // 启动一个新的线程，作为连接的生产者    linux thread=>pthread_create
    // std::bind绑定器，将类成员函数绑定在类对象上
    // TODO :为什么放在构造函数里面
    // 这里在单例模式的构造函数中，针对实例仅仅创建唯一的线程，详情见小红书的理解
    std::thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    // 分离线程
    produce.detach();

    // 启动一个新的线程，扫描超过 maxIdleTime 时间的空闲连接，进行对于多余connection的回收
    std::thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach();
}

// 运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (!_connectionQueue.empty())
        {
            cv.wait(lock); // 队列不空，此处生产线程进入等待状态
        }
        // 生产线程进行生产,连接数量为达到上限，继续创建新的连接
        if (_connectionCnt < _maxSize)
        {
            Connection *p = new Connection();
            if (p->connect(_ip, _port, _username, _password, _dbname))
            {
                p->refreshAliveTime(); // 刷新一下开始空闲的起始时间
                _connectionQueue.push(p);
                _connectionCnt++;
            }
        }
        // 通知消费者线程可以消费连接了
        cv.notify_all();
    }
}

// 给外部提供接口，从连接池中获取一个可用的空闲连接
std::shared_ptr<Connection> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    while (_connectionQueue.empty())
    {
        // 设置最大超时时间
        if (std::cv_status::timeout == cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout)))
        {
            if (_connectionQueue.empty())
            {
                LOG("获取空闲连接超时了。。。获取连接失败！");
                return nullptr;
            }
        }
    }

    /*
    shared_ptr智能指针析构时，会把connection资源直接delete掉，，相当于调用connection的析构函数
    connection就被close掉了。这里需要自定义shared_ptr的释放资源的方式

    把connection直接归还到queue当中
    */
    std::shared_ptr<Connection> sp(_connectionQueue.front(),
                                   [&](Connection *pcon)
                                   {
                                       // 这里是在服务器应用线程中进行调用的，所以一定要考虑队列的线程安全操作
                                       std::unique_lock<std::mutex> lock(_queueMutex);
                                       pcon->refreshAliveTime();
                                       _connectionQueue.push(pcon);
                                   });
    _connectionQueue.pop();
    if (_connectionQueue.empty())
        cv.notify_all(); // 谁消费了队列中的最后一个connection，谁负责通知生产者
    return sp;
}

// 扫描超过maxIdleTime时间的空闲连接，进行对于超时连接资源的回收
void ConnectionPool::scannerConnectionTask()
{
    for (;;)
    {
        // 通过sleep模拟定时效果
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

        // 扫描整个队列，释放多余的连接
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection *p = _connectionQueue.front();
            //_maxIdleTime 为s，需要转换为ms
            if (p->getAliveTime() >= (_maxIdleTime))
            {
                _connectionQueue.pop();
                _connectionCnt--;
                delete p;
            }
            //如果队头都未超时，那么整个队列都不会超过时间
            else
            {
                break;
            }
        }
    }
}
