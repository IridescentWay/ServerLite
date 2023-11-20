#ifndef __SERVERLITE_H__
#define __SERVERLITE_H__

#include "./http/http_conn.h"
#include "./thread_pool/threadpool.h"

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
    void timer(int connfd, struct sockaddr_in client_address);
    void adjustTimer(MTimer)
}

#endif
