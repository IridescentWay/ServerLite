#include "threadpool.h"

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
                    request->imporv = 1;
                    DB_ConnectionUniPtr mysqlConn(&request->mysql, m_connPool);
                    request->process;
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
            DB_ConnectionUniPtr mysqlConn(&request->mysql, m_connPool);
            request->process();
        }
    }
}

