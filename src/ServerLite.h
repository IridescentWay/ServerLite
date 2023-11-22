#ifndef __SERVERLITE_H__
#define __SERVERLITE_H__

#include "./http/http_conn.h"
#include "./thread_pool/threadpool.h"
#include "./utils/utils.h"
#include "db_connection_pool/db_connection_pool.h"

const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5;

class ServerLite
{
public:
    ServerLite();
    ~ServerLite();

    void init(int port, std::string user, std::string pswd, std::string dbName,
            int logWrite, int optLinger, int trigMode, int sqlNum, int threadNum,
            int logOption, enum ActorModel actorModel);

    void initThreadpool();
    void initSqlpool();
    void logWrite();
    void trigMode();
    void eventListen();
    void eventLoop();
    void createUserConnAndTimer(int connfd, struct sockaddr_in client_address);
    void adjustTimer(MTimer *timer);
    void dealwithTimer(MTimer *timer, int sockfd);
    bool dealwithClientData();
    bool dealwithSignal(bool &timeout, bool &stop_server);
    void dealwithRead(int sockfd);
    void dealwithWrite(int sockfd);

public:
    int m_port;
    char *m_root;
    int m_logWrite;
    int m_logOption;
    enum ActorModel m_actorModel;

    int m_pipefd[2];
    int m_epollfd;
    HttpConnection *users;

    // 数据库
    DB_ConnectionPool *m_connPool;
    std::string m_user;
    std::string m_pswd;
    std::string m_dbName;
    int m_sqlNum;

    // 线程池
    ThreadPool<HttpConnection> *m_threadpool;
    int m_threadNum;

    // epoll_event
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_optLinger;
    int m_trigMode;
    int m_listenTrigMode;
    int m_connTrigMode;

    // 定时器
    client_data *usersTimer;
    Utils utils;
};

#endif
