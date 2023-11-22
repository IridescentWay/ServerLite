#include "db_connection_pool.h"

#include <iterator>
#include <mysql/mysql.h>
#include <iostream>

DB_ConnectionPool* DB_ConnectionPool::getInstance()
{
    static DB_ConnectionPool connPool;
    return &connPool;
}

void DB_ConnectionPool::init(std::string url, std::string user, std::string pswd, std::string dbName, int port, int maxConn, int log_option)
{
    m_url = url;
    m_user = user;
    m_pswd = pswd;
    m_port = port;
    m_DBName = dbName;
    m_log_option = log_option;

    for (int i = 0; i < maxConn; ++i) {
        MYSQL *conn = NULL;
        conn = mysql_init(conn);
        if (NULL == conn) {
            std::cout << "MySQL init Error" << std::endl;
            exit(1);
        }
        conn = mysql_real_connect(conn, url.c_str(), user.c_str(), pswd.c_str(), dbName.c_str(), port, NULL, 0);
        if (NULL == conn) {
            std::cout << url.c_str() << std::endl << user.c_str() << std::endl << dbName.c_str() << std::endl << port << std::endl;
            std::cout << "MySQL real connect Error" << std::endl;
            exit(1);
        }
        connList.push_back(conn);
    }

    reserveSem = Semaphore(connList.size());
    m_MaxConn = connList.size();
}

MYSQL* DB_ConnectionPool::getConnection()
{
    MYSQL *conn = NULL;
    if (0 == connList.size()) {
        return NULL;
    }
    reserveSem.wait();
    lock.lock();
    conn = connList.front();
    connList.pop_front();
    lock.unlock();
    return conn;
}

bool DB_ConnectionPool::releaseConnection(MYSQL *conn)
{
    if (NULL == conn) {
        return false;
    }

    lock.lock();
    connList.push_back(conn);
    lock.unlock();
    reserveSem.post();
    return true;
}

void DB_ConnectionPool::destroyPool()
{
    lock.lock();
    if (connList.size() > 0) {
        for (auto conn : connList) {
            mysql_close(conn);
        }
        connList.clear();
    }
    lock.unlock();
}

int DB_ConnectionPool::getFreeConnNum()
{
    return connList.size();
}

DB_ConnectionPool::~DB_ConnectionPool()
{
    destroyPool();
}

DB_ConnectionUniPtr::DB_ConnectionUniPtr(MYSQL **conn, DB_ConnectionPool *connPool)
{
    *conn = connPool->getConnection();
    m_conn = *conn;
    m_connPool = connPool;
}

DB_ConnectionUniPtr::~DB_ConnectionUniPtr()
{
    m_connPool->releaseConnection(m_conn);
}
