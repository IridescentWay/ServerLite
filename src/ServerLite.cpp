#include "ServerLite.h"
#include "db_connection_pool/db_connection_pool.h"
#include "http/http_conn.h"
#include "thread_pool/threadpool.h"
#include "utils/utils.h"

#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>

ServerLite::ServerLite()
{
    // htpp connection 对象池
    users = new HttpConnection[MAX_FD];

    // root 文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char*) malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    // 定时器, 与 http 连接一一对应
    usersTimer = new client_data[MAX_FD];
}

ServerLite::~ServerLite()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete [] usersTimer;
    delete m_threadpool;
}

void ServerLite::init(int port, std::string user, std::string pswd, std::string dbName, 
    int logWrite, int optLinger, int trigMode, int sqlNum, int threadNum, int logOption, enum ActorModel actorModel)
{
    m_port = port;
    m_user = user;
    m_pswd = pswd;
    m_dbName = dbName;
    m_sqlNum = sqlNum;
    m_threadNum = threadNum;
    m_logWrite = logWrite;
    m_optLinger = optLinger;
    m_trigMode = trigMode;
    m_logOption = logOption;
    m_actorModel = actorModel;
}

void ServerLite::trigMode()
{
    m_listenTrigMode = (m_trigMode >> 1) & 1;
    m_connTrigMode = m_trigMode & 1;
}

void ServerLite::logWrite()
{
    if (0 == m_logOption) {

    }
}

void ServerLite::initSqlpool()
{
    // 初始化数据库连接池
    m_connPool = DB_ConnectionPool::getInstance();
    m_connPool->init("localhost", m_user, m_pswd, m_dbName, 3306, m_sqlNum, m_logOption);

    // 初始化数据库读取表
    users->initMySQLResult(m_connPool);
}

void ServerLite::initThreadpool()
{
    // 线程池
    m_threadpool = new ThreadPool<HttpConnection>(m_actorModel, m_connPool, m_threadNum);
}

void ServerLite::eventListen()
{
    // 网络编程基础步骤
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    // 优雅关闭连接
    struct linger tmp = {m_optLinger, 1};
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    // epoll 创建内核事件表
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    utils.addfd(m_epollfd, m_listenfd, false, m_listenTrigMode);
    HttpConnection::m_epollfd = m_epollfd;
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setNonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    utils.addSig(SIGPIPE, SIG_IGN);
    utils.addSig(SIGALRM, utils.sigHandler, false);
    utils.addSig(SIGTERM, utils.sigHandler, false);

    alarm(TIMESLOT);

    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

void ServerLite::eventLoop()
{
    bool timeout = false;
    bool stopServer = false;

    while (!stopServer) {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd) {
                // 处理新到的客户连接
                bool flag = dealwithClientData();
                if (false == flag) {
                    continue;
                }
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 服务器端关闭连接，移除对应的定时器
                MTimer *timer = usersTimer[sockfd].timer;
                dealwithTimer(timer, sockfd);
            } else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)) {
                // 处理信号
                bool flag = dealwithSignal(timeout, stopServer);
                if (false == flag) {
                    // LOG_ERROR
                }
            } else if (events[i].events & EPOLLIN) {
                printf("%s\n", "request received");
                dealwithRead(sockfd);
            } else if (events[i].events & EPOLLOUT) {
                dealwithWrite(sockfd);
            }
        }
        if (timeout) {
            utils.timerHandler();
            // LOG_INFO
            timeout = false;
        }
    }
}


void ServerLite::createUserConnAndTimer(int connfd, struct sockaddr_in client_address)
{
    users[connfd].init(connfd, client_address, m_root, m_connTrigMode, m_logOption, m_user, m_pswd, m_dbName);
    // 初始化 client_data 数据
    // 创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    usersTimer[connfd].address = client_address;
    usersTimer[connfd].sockfd = connfd;
    MTimer *timer = new MTimer;
    timer->userData = &usersTimer[connfd];
    timer->call_back = call_back;
    time_t current = time(NULL);
    timer->expire = current + 3 * TIMESLOT;
    usersTimer[connfd].timer = timer;
    utils.m_timerLst.addTimer(timer);
}

// 若连接上有数据传输，则更新定时器延迟三个单位并调整其在有序链表中的位置
void ServerLite::adjustTimer(MTimer *timer)
{
    time_t current = time(NULL);
    timer->expire = current + 3 * TIMESLOT;
    utils.m_timerLst.adjustTimer(timer);
}

void ServerLite::dealwithTimer(MTimer *timer, int sockfd)
{
    timer->call_back(&usersTimer[sockfd]);
    if (timer) {
        utils.m_timerLst.delTimer(timer);
    }
}

bool ServerLite::dealwithClientData()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (0 == m_listenTrigMode) {
        int connfd = accept(m_listenfd, (struct sockaddr*) &client_address, &client_addrlength);
        if (connfd < 0) {
            return false;
        }
        if (HttpConnection::m_userCtr >= MAX_FD) {
            utils.showError(connfd, "Internal server busy");
            return false;
        }
        createUserConnAndTimer(connfd, client_address);
    } else {
        while (1) {
            int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &client_addrlength);
            if (connfd < 0) {
                break;
            }
            if (HttpConnection::m_userCtr >= MAX_FD) {
                utils.showError(connfd, "Internal server busy");
                break;
            }
            createUserConnAndTimer(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool ServerLite::dealwithSignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (-1 == ret) {
        return false;
    } else if (0 == ret) {
        return false;
    } else {
        for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
                case SIGALRM:
                    timeout = true;
                    break;
                case SIGTERM:
                    stop_server = true;
                    break;
            }
        }
    }
    return true;
}

void ServerLite::dealwithRead(int sockfd)
{
    MTimer *timer = usersTimer[sockfd].timer;
    if (REACTOR == m_actorModel) {
        if (timer) {
            adjustTimer(timer);
        }
        users[sockfd].m_state = 0;  // read
        m_threadpool->append(users+sockfd);
        // reactor 为非阻塞同步网络模式，可以异步非阻塞感知到监听事件，之后分发给工作线程，工作线程处理 I/O 时需要同步阻塞
        while (true) {
            if (1 == users[sockfd].improv) {
                if (1 == users[sockfd].timer_flag) {
                    dealwithTimer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    } else {
        // Proactor 是异步网络模式，感知事件和读写都是异步非阻塞，读写事件完成后通过工作线程处理
        // 这里其实通过设置 ET 触发轮询读写操作完成属于模拟异步，并非真正异步
        if (users[sockfd].readOnce()) {
            m_threadpool->append(users+sockfd);
            if (timer) {
                adjustTimer(timer);
            }
        } else {
            dealwithTimer(timer, sockfd);
        }
    }
}

void ServerLite::dealwithWrite(int sockfd)
{
    MTimer *timer = usersTimer[sockfd].timer;
    if (REACTOR == m_actorModel) {
        if (timer) {
            adjustTimer(timer);
        }
        users[sockfd].m_state = 1;  // write
        m_threadpool->append(users+sockfd);
        while (true) {
            if (1 == users[sockfd].improv) {
                if (1 == users[sockfd].timer_flag) {
                    dealwithTimer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    } else {
        // Proactor 模式
        if (users[sockfd].write()) {
            if (timer) {
                adjustTimer(timer);
            }
        } else {
            dealwithTimer(timer, sockfd);
        }
    }
}
