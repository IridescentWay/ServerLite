#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "../db_connection_pool/db_connection_pool.h"

#include <cstdio>
#include <exception>

#include <pthread.h>

enum ActorModel {PROACTOR, REACTOR};

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
    std::list<T*> m_workqueue;
    Locker m_queuelocker;
    Semaphore m_queueStat;
    enum ActorModel m_actor_mode;
    DB_ConnectionPool *m_connPool;
    int m_threadNum;
    size_t m_maxRequests;
    pthread_t *m_threads;
};

template<typename T>
ThreadPool<T>::ThreadPool(enum ActorModel actor_model, DB_ConnectionPool *connPool, int threadNum, int maxRequests)
: m_actor_mode(actor_model), m_connPool(connPool), m_threadNum(threadNum), m_maxRequests(maxRequests), m_threads(NULL)
{
    if (threadNum <= 0 || maxRequests <= 0) {
        throw std::exception();
    }
    m_threads = new pthread_t[m_threadNum];
    if (NULL == m_threads) {
        throw std::exception();
    }
    for (int i = 0; i < m_threadNum; ++i) {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete [] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i])) {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete [] m_threads;
}

template <typename T>
bool ThreadPool<T>::append(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_maxRequests) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queueStat.post();
    return true;
}

template <typename T>
void* ThreadPool<T>::worker(void* arg)
{
    ThreadPool* pool = (ThreadPool*) arg;
    pool->run();
    return pool;
}

template <typename T>
void ThreadPool<T>::run()
{
    while (true) {
        m_queueStat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (request == NULL) {
            continue;
        }
        if (REACTOR == m_actor_mode) {
            if (0 == request->m_state) {
                if (request->readOnce()) {
                    request->improv = 1;
                    DB_ConnectionUniPtr mysqlConn(&request->mysql, m_connPool);
                    request->process();
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            } else {
                if (request->write()) {
                    request->improv = 1;
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        } else {
            printf("%s\n", "threadpool task executed");
            DB_ConnectionUniPtr mysqlConn(&request->mysql, m_connPool);
            request->process();
            printf("%s\n", "request processed");
        }
    }
}

#endif  // __THREADPOOL_H__
