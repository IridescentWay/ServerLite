#ifndef __HTTP_CONN_H__
#define __HTTP_CONN_H__

#include <arpa/inet.h>
#include <cstddef>
#include <fcntl.h>
#include <mysql/mysql.h>
#include <netinet/in.h>

#include <string>
#include <map>

#include "../db_connection_pool/db_connection_pool.h"

class HttpConnection
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        REQUEST_LINE,
        HEADER,
        CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK,
        LINE_BAD,
        LINE_OPEN
    };

public:
    HttpConnection();
    ~HttpConnection();

public:
    void init(int sockfd, const sockaddr_in &addr, char* root, int TrigMode, int log_option, std::string user, std::string pswd, std::string sql_name);
    void close_conn(bool real_close = true);
    void process();
    bool readOnce();
    bool write();
    sockaddr_in *getAddress();
    void initMySQLResult(DB_ConnectionPool *connPool);
    
    int timer_flag;
    int improv;

private:
    void init();
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE readRet);
    HTTP_CODE parseRequestLine(char *text);
    HTTP_CODE parseHeaders(char *text);
    HTTP_CODE parseContent(char *text);
    HTTP_CODE doRequest();
    char* getLine();
    LINE_STATUS parseLine();
    void unmap();
    bool addResponse(const char *format, ...);
    bool addContent(const char *content);
    bool addStatusLine(int status, const char *title);
    bool addHeaders(int contentLen);
    bool addContentType();
    bool addContentLength(int contentLen);
    bool addLinget();
    bool addBlankLine();

public:
    static int m_epollfd;
    static int m_userCtr;
    MYSQL *mysql;
    int m_state;

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_readBuf[READ_BUFFER_SIZE];
    long m_read_idx;
    long m_checked_idx;
    int m_start_line;
    char m_writeBuf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_checkState;
    METHOD m_method;
    char m_realFile[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    long m_contentLen;
    bool m_linger;
    char *m_fileAddress;
    struct stat m_fileStat;
    struct iovec m_iv[2];
    int m_iv_ctr;
    int cgi;
    char *m_requestHeader;
    int bytesToSend;
    size_t bytesHaveSent;
    char *doc_root;

    std::map<std::string, std::string> m_users;
    int m_TrigMode;
    int m_logOption;

    char sqlUser[100];
    char sqlPswd[100];
    char sqlName[100];
};

#endif  // __HTTP_CONN_H__
