#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "../db_connection_pool/db_connection_pool.h"

enum ActorModel {Reactor, Proactor};

template <typename T>
class ThreadPool
{
public:
    ThreadPool(enum ActorModel actor_model, DB_ConnectionPool *connPool, int threadNum = 8, int maxRequest = 10000);
    ~ThreadPool();
    bool append(T *request);

private:
    int m_threadNum;
    int m_maxRequests;
    pthread_t *m_threads;
    std::list<T*> m_workqueue;
    Locker m_queuelocker;
    Cond_Variable m_hasWork;
    
};

#endif  // __THREADPOOL_H__
