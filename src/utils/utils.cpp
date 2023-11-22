#include "utils.h"
#include "../http/http_conn.h"

#include <sys/epoll.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

MTimer::MTimer() : prev(NULL), next(NULL) {}

TimerSortedLst::TimerSortedLst() : head(NULL), tail(NULL) {}

TimerSortedLst::~TimerSortedLst()
{
    MTimer *tmp = head;
    while (tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void TimerSortedLst::addTimer(MTimer *timer)
{
    if (!timer) {
        return ;
    }
    if (!head) {
        head = tail = timer;
        return ;
    }
    if (timer->expire < head->expire) {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return ;
    }
    addTimer(timer, head);
}

void TimerSortedLst::addTimer(MTimer *timer, MTimer *lstHead)
{
    MTimer *prev = lstHead;
    MTimer *tmp = prev->next;
    while (tmp) {
        if (timer->expire < tmp->expire) {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp) {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void TimerSortedLst::adjustTimer(MTimer *timer)
{
    if (!timer) {
        return ;
    }
    MTimer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire)) {
        return ;
    }
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        addTimer(timer, head);
    } else {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        addTimer(timer, timer->next);
    }
}

void TimerSortedLst::delTimer(MTimer *timer)
{
    if (!timer) {
        return ;
    }
    if ((timer == head) && (timer == tail)) {
        delete timer;
        head = NULL;
        tail = NULL;
        return ;
    }
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return ;
    }
    if (timer == tail) {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return ;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void TimerSortedLst::tick()
{
    if (!head) {
        return ;
    }
    time_t current = time(NULL);
    MTimer *tmp = head;
    while (tmp) {
        if (current < tmp->expire) {
            break;
        }
        tmp->call_back(tmp->userData);
        head = tmp->next;
        if (!head) {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

Utils::Utils() {}

Utils::~Utils() {}

void Utils::init(int timeslot)
{
    m_timeslot = timeslot;
}

int Utils::setNonblocking(int fd)
{
    int oldOpt = fcntl(fd, F_GETFL);
    int newOpt = oldOpt | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOpt);
    return oldOpt;
}

// 将内核时间表注册读事件，选择开启 ET 模式、ONESHOT 模式
void Utils::addfd(int epollfd, int fd, bool oneshot, int trigMode)
{
    epoll_event event;
    event.data.fd = fd;

    event.events = EPOLLIN | EPOLLRDHUP;

    if (1 == trigMode) {
        event.events |= EPOLLET;
    }
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);
}

void Utils::removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void Utils::modfd(int epollfd, int fd, int ev, int trigMode)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    if (1 == trigMode) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void Utils::sigHandler(int sig)
{
    send(u_pipefd[1], (char*)&sig, 1, 0);
}

void Utils::addSig(int sig, void (*handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::timerHandler()
{
    m_timerLst.tick();
    alarm(m_timeslot);
}

void Utils::showError(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

void call_back(client_data *userData)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, userData->sockfd, 0);
    assert(userData);
    close(userData->sockfd);
    --HttpConnection::m_userCtr;
}
