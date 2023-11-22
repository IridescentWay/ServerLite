#include "./config/config.h"
#include "ServerLite.h"

int main(int argc, char *argv[]) {
    // 数据库信息
    std::string user = "admin";
    std::string pswd = "7428@0203";
    std::string dbName = "server_db";

    // 命令行解析
    Config config;
    config.parse_arg(argc, argv);

    ServerLite server;
    server.init(config.PORT, user, pswd, dbName, 
        config.LOGWrite, config.OPT_LINGER, config.TRIGMode, 
        config.SQL_NUM, config.THREAD_NUM, config.LOG_OPT, config.ACTOR_MODEL);

    server.logWrite();
    server.initSqlpool();
    server.initThreadpool();
    server.trigMode();
    server.eventListen();
    server.eventLoop();
    return 0;
}