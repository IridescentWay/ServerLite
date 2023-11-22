#include "config.h"
#include <bits/getopt_core.h>

Config::Config(){
    //端口号,默认9006
    PORT = 9006;

    //日志写入方式，默认同步
    LOGWrite = 0;

    //触发组合模式,默认listenfd LT + connfd LT
    TRIGMode = 0;

    //listenfd触发模式，默认LT
    LISTENTrigmode = 0;

    //connfd触发模式，默认LT
    CONNfdTrigmode = 0;

    //优雅关闭链接，默认不使用
    OPT_LINGER = 0;

    //数据库连接池数量,默认8
    SQL_NUM = 8;

    //线程池内的线程数量,默认8
    THREAD_NUM = 8;

    //关闭日志,默认关闭
    LOG_OPT = 0;

    //并发模型,默认是proactor
    ACTOR_MODEL = PROACTOR;
}

void Config::parse_arg(int argc, char*argv[]){
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            PORT = atoi(optarg);
            break;
        }
        case 'l':
        {
            LOGWrite = atoi(optarg);
            break;
        }
        case 'm':
        {
            TRIGMode = atoi(optarg);
            break;
        }
        case 'o':
        {
            OPT_LINGER = atoi(optarg);
            break;
        }
        case 's':
        {
            SQL_NUM = atoi(optarg);
            break;
        }
        case 't':
        {
            THREAD_NUM = atoi(optarg);
            break;
        }
        case 'c':
        {
            LOG_OPT = atoi(optarg);
            break;
        }
        case 'a':
        {
            ACTOR_MODEL = (enum ActorModel) atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}
