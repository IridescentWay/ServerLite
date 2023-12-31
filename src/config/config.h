#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "../ServerLite.h"

class Config
{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char*argv[]);

    //端口号
    int PORT;

    //日志写入方式
    int LOGWrite;

    //触发组合模式
    int TRIGMode;

    //listenfd触发模式
    int LISTENTrigmode;

    //connfd触发模式
    int CONNfdTrigmode;

    //优雅关闭链接
    int OPT_LINGER;

    //数据库连接池数量
    int SQL_NUM;

    //线程池内的线程数量
    int THREAD_NUM;

    //是否关闭日志
    int LOG_OPT;

    //并发模型选择
    enum ActorModel ACTOR_MODEL;
};

#endif