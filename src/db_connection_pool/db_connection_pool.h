#ifndef __CONNECTION_POOL__
#define __CONNECTION_POOL__

#include <string>
#include <list>

#include <mysql/mysql.h>

#include "../locker/locker.h"

class DB_ConnectionPool
{
public:
    MYSQL *getConnection();
    bool releaseConnection(MYSQL *conn);
    int getFreeConnNum();
    void destroyPool();

    static DB_ConnectionPool *getInstance();

    void init(std::string url, std::string user, std::string pswd, std::string dbName, int port, int maxConn, int log_option);

private:
    ~DB_ConnectionPool();

    int m_MaxConn;  // 最大连接数
    Locker lock;
    std::list<MYSQL*> connList;  // 链表管理连接池
    Semaphore reserveSem;

public:
    std::string m_url;
    std::string m_port;
    std::string m_user;
    std::string m_pswd;
    std::string m_DBName;
    int m_log_option;
};

class DB_ConnectionUniPtr {
public:
    DB_ConnectionUniPtr(MYSQL **conn, DB_ConnectionPool *connPool);
    ~DB_ConnectionUniPtr();

private:
    MYSQL *m_conn;
    DB_ConnectionPool *m_connPool;
};

#endif  // __CONNECTION_POOL__