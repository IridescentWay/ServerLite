#ifndef __LOCKER_H__
#define __LOCKER_H__

#include <semaphore.h>

class Semaphore
{
public:
    Semaphore();
    Semaphore(int num);
    ~Semaphore();

    bool wait();
    bool post();
    int getValue();

private:
    sem_t m_sem;
};

class Locker
{
public:
    Locker();
    ~Locker();

    bool lock();
    bool unlock();
    pthread_mutex_t* getMutex();

private:
    pthread_mutex_t m_mutex;
};

class Cond_Variable
{
public:
    Cond_Variable();
    ~Cond_Variable();

    bool signal();
    bool broadcast();
    bool wait(Locker &locker);
    bool timedwait(Locker &locker, struct timespec &t);

private:
    pthread_cond_t m_cond;
};

#endif
