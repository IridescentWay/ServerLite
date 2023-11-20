#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "../db_connection_pool/db_connection_pool.h"

#include <exception>

#include <pthread.h>

enum ActorModel {REACTOR, PROACTOR};

template <typename T>
class ThreadPool
{
public:
    ThreadPool(enum ActorModel actor_model, DB_ConnectionPool *connPool, int threadNum = 8, int maxRequest = 10000);
    ~ThreadPool();
    bool append(T *request);

private:
    static void *worker(void *arg);
    void run();

private:
    int m_threadNum;
    int m_maxRequests;
    pthread_t *m_threads;
    std::list<T*> m_workqueue;
    Locker m_queuelocker;
    Semaphore m_queueStat;
    DB_ConnectionPool *m_connPool;
    enum ActorModel m_actor_mode;
};

#endif  // __THREADPOOL_H__
