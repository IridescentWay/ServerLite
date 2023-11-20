#include "utils.h"

#include <stddef.h>

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
