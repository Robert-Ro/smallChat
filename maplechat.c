#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/select.h>

#define MAX_CLIENTS 1000 // 文件描述符，句柄？
#define MAX_NICK_LEN 32
#define SERVER_PORT 7712

struct client
{
    int fd;
    char *nick;
};

struct chatState
{
    int serversock;
    int numclients;
    int maxclient;
    struct client *clients[MAX_CLIENTS];
};

struct chatState *Chat;

/* Create a TCP socket lisetning to 'port' ready to accept connections. */
int createTCPServer(int port)
{
    int s, yes = 1;
    struct sockaddr_in sa;
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); // &操作符：取变量地址
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    // fixme
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) == -1 ||
        listen(s, 511) == -1)
    {
        close(s);
        return -1;
    }

    return s;
}
void *chatMalloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        perror("Out of memory");
        exit(1);
    }
    return ptr;
}
// FIXME fd关联socket？
int socketSetNonBlockNoDelay(int fd)
{
    int flags, yes = 1;

    /* Set the socket nonblocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1)
        return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;

    /* This is best-effort. No need to check for errors. */
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
    return 0;
}
/*
 *如果监听套接字指示有一个新的连接准备好被接受，我们接受它（accept(2)），在发生错误时返回-1，在成功时返回新的客户端套接字。
 */
int acceptClient(int server_socket)
{
    int s;
    while (1)
    {
        struct sockaddr_in sa;
        socklen_t slen = sizeof(sa);
        s = accept(server_socket, (struct sockaddr *)&sa, &slen);
        if (s == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        break;
    }
    return s;
}

struct client *createClient(int fd)
{
    char nick[32];
    int nicklen = snprintf(nick, sizeof(nick), "user:%d", fd);
    struct client *c = chatMalloc(sizeof(*c));
    socketSetNonBlockNoDelay(fd);
    c->fd = fd;
    c->nick = chatMalloc(nicklen + 1); // FIXME
    assert(Chat->clients[c->fd] == NULL);
    Chat->clients[c->fd] = c;
    // 更新最大客户端
    if (c->fd > Chat->maxclient)
    {
        Chat->maxclient = c->fd;
    }
    Chat->numclients++;
    return c;
}
void freeClient(struct client *c)
{
    free(c->nick);
    close(c->fd);
    Chat->clients[c->fd] = NULL;
    Chat->numclients--;
    // FIXME
    free(c);
}
void initChat(void)
{
    Chat = chatMalloc(sizeof(*Chat));
    memset(Chat, 0, sizeof(*Chat));
    Chat->maxclient = -1;
    Chat->numclients = 0;

    int tcpServer = createTCPServer(SERVER_PORT);
    if (tcpServer == -1)
    {
        perror("创建tcp服务器出错");
        exit(1);
    }
    Chat->serversock = tcpServer;
}

int main()
{
    initChat();
    while (1)
    {
        // 接收连接
        // 接收消息
        // 广播消息
    }

    return 0;
}