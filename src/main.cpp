#include "CommonConnectionPool.h"
#include "pch.h"
#include "public.h"

int main()
{
#if 1
    clock_t begin = clock();
    for (int i = 0; i < 1000; i++)
    {
        Connection conn;
        char sql[1024] = {0};
        sprintf(sql, "insert into user(name,age,sex) values('%s','%d','%s')",
                "zhang san", 20, "male");
        conn.connect("127.0.0.1", 3306, "root", "mysql", "chat");
        conn.update(sql);
    }
    clock_t end = clock();
    std::string result = "消耗时间为："+std::to_string((end-begin)/CLOCKS_PER_SEC)+"s";
    LOG(result);
#else

#endif
    return 0;
}