#include <exception>
#include <pthread.h>

#include "locker.h"

Semaphore::Semaphore()
{
    if (sem_init(&m_sem, 0, 0) != 0) {
        throw std::exception();
    }
}

Semaphore::Semaphore(int num)
{
    if (sem_init(&m_sem, 0, num) != 0) {
        throw std::exception();
    }
}

Semaphore::~Semaphore()
{
    sem_destroy(&m_sem);
}

bool Semaphore::wait()
{
    return sem_wait(&m_sem) == 0;
}

bool Semaphore::post()
{
    return sem_post(&m_sem) == 0;
}

int Semaphore::getValue()
{
    int val;
    sem_getvalue(&m_sem, &val);
    return val;
}

Locker::Locker()
{
    if (pthread_mutex_init(&m_mutex, NULL) != 0) {
        throw std::exception();
    }
}

Locker::~Locker()
{
    pthread_mutex_destroy(&m_mutex);
}

bool Locker::lock()
{
    return pthread_mutex_lock(&m_mutex) == 0;
}

bool Locker::unlock()
{
    return pthread_mutex_unlock(&m_mutex) == 0;
}

pthread_mutex_t* Locker::getMutex()
{
    return &m_mutex;
}

Cond_Variable::Cond_Variable()
{
    if (pthread_cond_init(&m_cond, NULL) != 0) {
        throw std::exception();
    }
}

Cond_Variable::~Cond_Variable()
{
    pthread_cond_destroy(&m_cond);
}

bool Cond_Variable::signal()
{
    return pthread_cond_signal(&m_cond) == 0;
}

bool Cond_Variable::broadcast()
{
    return pthread_cond_broadcast(&m_cond) == 0;
}

bool Cond_Variable::wait(Locker &locker)
{
    return pthread_cond_wait(&m_cond, locker.getMutex()) == 0;
}

bool Cond_Variable::timedwait(Locker &locker, struct timespec &t)
{
    return pthread_cond_timedwait(&m_cond, locker.getMutex(), &t);
}
