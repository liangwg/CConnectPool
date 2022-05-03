#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "mysql/mysql.h"
#include "CConnectPool.h"

void* threadFunc1(void* pNum)
{
    //调用阻塞函数从连接池取一个连接，占用15秒钟后释放
    MYSQL *pConn = CConnectPool::GetConnect();
    if(pConn)
    {
        printf("get connect ok! id: %d\n",*(int*)pNum);
    }
    else
    {
        printf("get connect error!id: %d\n",*(int*)pNum);
    }
    sleep(10);
    if(NULL != pConn)
    {
        CConnectPool::PutConnect(pConn);
    } 
    delete pNum;
}

void* threadFunc2(void* pNum)
{
    //非阻塞从连接池取一个连接，占用15秒钟后释放
    MYSQL *pConn = CConnectPool::TryGetConnect();
    if(pConn)
    {
        printf("get connect ok!id: %d\n",*(int*)pNum);
    }
    else
    {
        printf("get connect error!id: %d\n",*(int*)pNum);
    }
    sleep(10);
    if(NULL != pConn)
    {
        CConnectPool::PutConnect(pConn);
    }  
    delete pNum;
}



int main()
{
    /*wgl_wt: 为啥，在主线程中初始化实例化，在子线程中可以访问到？
      wgl: 这个问题问的好，主线程中对连接池进行实例化+初始化，指向该实例化对象的是类的静态成员变量指针s_pInstance，而静态的都是和类进行绑定的。
      只要本身不销毁，其他地方是直接可以用类名来访问该静态成员的。
      在函数实现上通过静态成员函数访问指向实例化对象的s_pInstance，再通过s_pInstance来访问非静态成员函数和非静态成员变量。秒啊，记录的点！edit
     */
    //主线程中调用初始化连接池函数
    CConnectPool::CreatePool();
    pthread_t threads[15];
    //编号
    int *pNum = NULL;
    //奇数线程调用阻塞函数从连接池获取空闲连接
    //偶数线程调用非阻塞函数从连接池获取空闲连接
    for(int i=0;i<15;++i)
    {
        pNum = new int;
        *pNum = i;
        if(i%2 == 0)
        {
            pthread_create(&threads[i],NULL,threadFunc1,pNum);
        }
        else
        {
            pthread_create(&threads[i],NULL,threadFunc2,pNum); 
        }
    }
    for(int i=0;i<15;++i)
    {
        pthread_join(threads[i],NULL);
    }

    //主线程中释放连接池
    CConnectPool::FreePool();
    printf("over \n");
    return 0;
}
