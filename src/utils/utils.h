#ifndef __UTILS_H__
#define __UTILS_H__

#include <bits/types/time_t.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <assert.h>
#include <signal.h>

class MTimer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    MTimer *timer;
};

class MTimer
{
public:
    MTimer();

public:
    time_t expire;

    void (*call_back)(client_data*);
    client_data *userData;
    MTimer *prev;
    MTimer *next;
};

class TimerSortedLst
{
public:
    TimerSortedLst();
    ~TimerSortedLst();

    void addTimer(MTimer *timer);
    void adjustTimer(MTimer *timer);
    void delTimer(MTimer *timer);
    void tick();

private:
    void addTimer(MTimer *timer, MTimer *lstHead);
    
    MTimer *head;
    MTimer *tail;
};

class Utils
{
public:
    Utils();
    ~Utils();

    void init(int timeslot);
    static int setNonblocking(int fd);
    static void addfd(int epollfd, int fd, bool oneshot, int trigMode);
    static void removefd(int epollfd, int fd);
    static void modfd(int epollfd, int fd, int ev, int trigMode);
    static void sigHandler(int sig);
    void addSig(int sig, void(handler)(int), bool restart = true);
    void timerHandler();
    void showError(int connfd, const char *info);

public:
    static int *u_pipefd;
    TimerSortedLst m_timerLst;
    static int u_epollfd;
    int m_timeslot;
};

void call_back(client_data *userData);

#endif
