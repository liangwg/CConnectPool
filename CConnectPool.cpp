#include "CConnectPool.h"

#define POOL_SAFE_DELETE(ptr) \
    if(ptr != NULL)\
    {\
        delete ptr;\
        ptr = NULL;\
    }

CConnectPool* CConnectPool::s_pInstance = NULL;

CConnectPool::CConnectPool()
{
    pthread_mutex_init(&m_mutex,NULL);
    pthread_cond_init(&m_cond,NULL);
    //初始化连接数
    //此处由主线程调用初始化，无需考虑锁问题
    for(int i=0;i<MYSQL_POOL_MIN_CONNECT;++i)
    {
        MYSQL *pConn = _CreateConnect();
        if(NULL == pConn)
        {
            exit(EXIT_FAILURE);
        }
        m_queueFree.push(pConn);
    }
}

CConnectPool::~CConnectPool()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
    MYSQL* pConn = NULL;
    while(m_queueFree.size()>0)
    {
        pConn = m_queueFree.front();
        POOL_SAFE_DELETE(pConn);
        m_queueFree.pop();
    }
    std::set<MYSQL*>::iterator it = m_setBusy.begin();
    while(it != m_setBusy.end())
    {
        pConn = *it;
        POOL_SAFE_DELETE(pConn);
        it = m_setBusy.erase(it);
    }
}


void CConnectPool::CreatePool()
{
    if(NULL == s_pInstance)
    {
        s_pInstance = new CConnectPool;
    }
}

void CConnectPool::FreePool()
{
    if(NULL != s_pInstance)
    {
        delete s_pInstance;
        s_pInstance = NULL;
    }
}

MYSQL* CConnectPool::GetConnect()
{
    return s_pInstance->_GetConnect(false);
}

MYSQL* CConnectPool::TryGetConnect()
{
    return s_pInstance->_GetConnect(true);
}

void CConnectPool::PutConnect(MYSQL*pConn)
{
    s_pInstance->_PutConnect(pConn);
}

MYSQL* CConnectPool::_CreateConnect()
{
    MYSQL* pConn = new MYSQL;
    mysql_init(pConn);
    if(!mysql_real_connect(pConn,MYSQL_HOST,MYSQL_USR,MYSQL_PWD,MYSQL_DB,MYSQL_PORT,NULL,CLIENT_FOUND_ROWS))
    {
        printf("connect error:%s\n",mysql_error(pConn));
        delete pConn;
        pConn = NULL;
    }
    return pConn;
}

MYSQL* CConnectPool::_GetConnect(bool bTry)
{
    static bool bCheck;
    MYSQL *pRet = NULL;
    pthread_mutex_lock(&m_mutex);
    //取出来还要检查一下是否能正常使用
    bCheck = true;
    if(m_queueFree.size()>0)
    {
        pRet = m_queueFree.front();
        m_queueFree.pop();
    }
    else
    {
        //连接数不足，继续创建
        if(m_queueFree.size() + m_setBusy.size()<MYSQL_POOL_MAX_CONNECT)
        {
            pRet = _CreateConnect();
            bCheck = false;
        }
        else
        {
            if(!bTry)
            {
                //阻塞等待空闲连接
                pthread_cond_wait(&m_cond,&m_mutex);
                //肯定会有空闲连接的
                if(m_queueFree.size()>0)
                {
                    pRet = m_queueFree.front();
                    m_queueFree.pop();
                }
            }
        }
    }
    //检查连接是否可用
    if(NULL != pRet && true == bCheck && 0 != mysql_ping(pRet))
    {
        mysql_close(pRet);
        pRet = _CreateConnect();
    }
    //加入busy队列
    if(NULL != pRet)
    {    
        m_setBusy.insert(pRet);
    }
    pthread_mutex_unlock(&m_mutex);
    return pRet;
}

void CConnectPool::_PutConnect(MYSQL* pConn)
{
    if(NULL == pConn)
        return;
    pthread_mutex_lock(&m_mutex);
    std::set<MYSQL*>::iterator it = m_setBusy.find(pConn);
    if(it != m_setBusy.end())
    {
        m_setBusy.erase(it);
    }
    m_queueFree.push(pConn);
    if(1 == m_queueFree.size())
    {
        //唤醒等待线程
        pthread_cond_signal(&m_cond);
    }
    pthread_mutex_unlock(&m_mutex);
}