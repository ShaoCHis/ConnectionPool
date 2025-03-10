**连接池主要包含了以下功能点：**

1. 连接池只需要一个实例，所以ConnectionPool以单例模式进行设计


2. 从ConnectionPool中可以获取和MySQL连接的Connection


3. 空闲连接Connection全部维护在一个线程安全的Connection队列中，使用线程互斥锁保证队列的线程安全


4. 如果Connection队列为空，还需要再获取连接，此时需要动态创建连接，上限数量是maxSize


5. 队列中空闲连接时间超过maxIdleTime的就要被释放掉，只保留初始的initSize个连接就可以了，**这个功能点肯定需要在独立的线程中去做**


6. 如果Connection队列为空，而此时连接的数量已达上限maxSize，那么等待connectionTimeout时间如果还获取不到空闲的连接，那么获取连接失败，此处从Connection队列获取空闲连接，可以使用带超时时间的mutex互斥锁来实现连接超时时间


7. 用户获取的连接用shared_ptr智能指针来管理，用lambda表达式定制连接释放的功能（不真正释放连接，而是把连接归还到连接池中）


8. 连接的生产和连接的消费采用生产者-消费者线程模型来设计，使用了线程间的同步通信机制条件变量和互斥锁


ps：个人在做这个项目时，思考于单例模式下多用户访问的过程以及构造函数中创建分离线程对连接池中的连接实例进行扫描定时回收机制，以下是一些个人思考理解

最近在思考池式结构工作原理。

我的理解下，多用户访问单例模式的池式结构（假设这里使用队列来存储与数据库的连接），所谓池式结构的唯一实例实际上是堆上的空间，存有对象的相关信息。然后在用户访问构成服务器的连接线程下，去访问该空间并执行相关代码信息，实际上是在各自的线程空间中执行相关操作，互不干扰，在访问临界资源时（队列）时需要进行加锁操作。这里的结构实际上可以使用Boost库下的无锁队列以及RCU来做。
	
另一个问题，如果我在单例模式的构造函数里面创建一个额外的工作线程用来扮演生产者线程，这个线程应该只被创建一个，再提供一个接口供用户拿取连接，那么这种情况实际上对应一个生产者，多个消费者，标准的spmc(single product multi consumer)结构
