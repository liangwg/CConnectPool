#ifndef CCONNECT_POOL_H_2021
#define CCONNECT_POOL_H_2021

#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <set>
#include <pthread.h>
#include <unistd.h>

#include "mysql/mysql.h"


//连接池最低连接数
#define MYSQL_POOL_MIN_CONNECT 2

//连接池最多连接数，超过该连接数需要等待有空闲连接
#define MYSQL_POOL_MAX_CONNECT 4

//数据库主机
#define MYSQL_HOST "127.0.0.1"

//数据库主机端口
#define MYSQL_PORT 3306

//数据库名称
#define MYSQL_DB   "test1"

//登录账号
#define MYSQL_USR  "root"

//登录密码
#define MYSQL_PWD  "111111"

class CConnectPool
{
public:

    /**
    * @brief 创建连接池 \n
    * 在主线程中创建连接池，创建后需要调用FreePool予以释放
    * 创建失败程序会打印错误信息并终止运行
    */
    static void CreatePool();

    /**
    * @brief 释放连接池 \n
    * 需要在主线程中调用该接口
    */
    static void FreePool();

    /**
    * @brief 获取连接 \n
    * 支持多线程，使用完成后需调用PutConnect，归还连接，本函数为阻塞函数，直到有可用连接才调用返回
    * @return 返回数据库连接
    *  --NULL表示调用失败
    */
    static MYSQL* GetConnect();

    /**
    * @brief 获取连接 \n
    * 支持多线程，使用完成后需调用PutConnect，归还连接，本函数调用后立刻返回，若没有空闲连接则返回NULL
    * @return 返回数据库连接
    *  --NULL表示调用失败
    * */
    static MYSQL* TryGetConnect();

    /**
    * @brief 向连接池归还连接 \n
    * 支持多线程
    * @param[in] pConn    需要归还的数据库连接
    * */
    static void PutConnect(MYSQL*pConn);

private:
    CConnectPool();
    ~CConnectPool();
    MYSQL* _GetConnect(bool bTry);
    void _PutConnect(MYSQL* pConn);
    //真正创建连接
    MYSQL* _CreateConnect();

    static CConnectPool* s_pInstance;
    std::queue<MYSQL*> m_queueFree;//空闲连接队列
    std::set<MYSQL*> m_setBusy;//被占用连接集 ，每个正在被使用的数据库连接随时都有可能释放连接
    pthread_mutex_t m_mutex;//为什么只用一个m_mutex?这个m_mutex是针对m_queueFree和m_setBusy的，两个一起操作，条件的判断也是和这两个有关
    pthread_cond_t m_cond;  //用m_cond的原因：连接池的连接数达到最大时，想获取连接的线程和准备释放连接的线程构成条件变量组，
};

#endif
