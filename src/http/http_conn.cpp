#include "http_conn.h"
#include "../utils/utils.h"

#include <asm-generic/errno-base.h>
#include <cstdio>
#include <errno.h>
#include <string.h>
#include <mysql/mysql.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/mman.h>

Locker m_lock;

int HttpConnection::m_userCtr = 0;
int HttpConnection::m_epollfd = -1;

HttpConnection::HttpConnection() {}

HttpConnection::~HttpConnection() {}

void HttpConnection::init()
{
    mysql = NULL;
    bytesToSend = 0;
    bytesHaveSent = 0;
    m_checkState = REQUEST_LINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_contentLen = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_readBuf, '\0', READ_BUFFER_SIZE);
    memset(m_writeBuf, '\0', WRITE_BUFFER_SIZE);
    memset(m_realFile, '\0', FILENAME_LEN);
}

void HttpConnection::init(int sockfd, const sockaddr_in &addr, char *root, int TrigMode, int log_option, std::string user, std::string pswd, std::string sql_name)
{
    m_sockfd = sockfd;
    m_address = addr;

    Utils::addfd(m_epollfd, sockfd, true, m_TrigMode);
    ++m_userCtr;

    doc_root = root;
    m_TrigMode = TrigMode;
    m_logOption = log_option;

    strcpy(sqlUser, user.c_str());
    strcpy(sqlPswd, pswd.c_str());
    strcpy(sqlName, sql_name.c_str());
    init();
}

void HttpConnection::initMySQLResult(DB_ConnectionPool *connPool)
{
    MYSQL *mysql = NULL;
    DB_ConnectionUniPtr mysqlconn(&mysql, connPool);

    if (mysql_query(mysql, "SELECT username,password FROM user")) {
        // LOG_ERROR
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    // int num_fileds = mysql_num_fields(result);
    // MYSQL_FIELD *fields = mysql_fetch_fields(result);
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        std::string k_user(row[0]);
        std::string v_pswd(row[1]);
        m_users[k_user] = v_pswd;
    }
}

void HttpConnection::close_conn(bool realClose)
{
    if (realClose && (m_sockfd != -1)) {
        Utils::removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        --m_userCtr;
    }
}

bool HttpConnection::readOnce()
{
    if (m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytesRead = 0;

    if (0 == m_TrigMode) {
        // LT 读取数据
        bytesRead = recv(m_sockfd, m_readBuf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytesRead;
        if (bytesRead <= 0) {
            return false;
        }
        return true;
    } else {
        // ET 读取数据
        while (true) {
            bytesRead = recv(m_sockfd, m_readBuf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if (-1 == bytesRead) {
                if (EAGAIN == errno || EWOULDBLOCK == errno) {
                    break;
                }
                return false;
            } else if (0 == bytesRead) {
                return false;
            }
            m_read_idx += bytesRead;
        }
        return true;
    }
}

void HttpConnection::unmap()
{
    if (m_fileAddress) {
        munmap(m_fileAddress, m_fileStat.st_size);
        m_fileAddress = 0;
    }
}

bool HttpConnection::write()
{
    int temp = 0;
    if (0 == bytesToSend) {
        Utils::modfd(m_epollfd, m_sockfd, EPOLLIN, m_TrigMode);
        init();
        return true;
    }
    while (true) {
        temp = writev(m_sockfd, m_iv, m_iv_ctr);
        if (temp < 0) {
            if (EAGAIN == errno) {
                Utils::modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TrigMode);
                return false;
            }
            unmap();
            return false;
        }

        bytesHaveSent += temp;
        bytesToSend -= temp;
        if (bytesHaveSent >= m_iv[0].iov_len) {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_fileAddress + (bytesHaveSent - m_write_idx);
            m_iv[1].iov_len = bytesToSend;
        } else {
            m_iv[0].iov_base = m_writeBuf + bytesHaveSent;
            m_iv[0].iov_len = m_iv[0].iov_len - bytesHaveSent;
        }
        if (bytesToSend <= 0) {
            unmap();
            Utils::modfd(m_epollfd, m_sockfd, EPOLLIN, m_TrigMode);
            if (m_linger) {
                init();
                return true;
            } else {
                return false;
            }
        }
    }
}

void HttpConnection::process()
{
    printf("%s\n", "read task processed\n");
    HTTP_CODE readRet = processRead();
    if (NO_REQUEST == readRet) {
        Utils::modfd(m_epollfd, m_sockfd, EPOLLIN, m_TrigMode);
        return ;
    }
    bool writeRet = processWrite(readRet);
    if (!writeRet) {
        close_conn();
    }
    printf("%s\n", "response sent");
    Utils::modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TrigMode);
}

HttpConnection::HTTP_CODE HttpConnection::processRead()
{
    return FILE_REQUEST;
}

bool HttpConnection::processWrite(HTTP_CODE readRet)
{
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nHello, Client!";
    int len = snprintf(m_writeBuf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, "%s", response.c_str());
    m_write_idx += len;
    m_iv[0].iov_base = m_writeBuf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_ctr = 1;
    bytesToSend = m_write_idx;
    return true;
}
