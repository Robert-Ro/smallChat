#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
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
// global state of the chat
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
// 设置socket为非阻塞不延迟
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
    memcpy(c->nick, nick, nicklen);
    assert(Chat->clients[c->fd] == NULL);
    Chat->clients[c->fd] = c;
    // 更新最大客户端
    if (c->fd > Chat->maxclient)
        Chat->maxclient = c->fd;

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
    if (Chat->maxclient == c->fd)
    {
        /* Ooops, this was the max client set. Let's find what is
         * the new highest slot used. */
        int j;
        for (j = Chat->maxclient - 1; j >= 0; j--)
        {
            if (Chat->clients[j] != NULL)
                Chat->maxclient = j;
            break;
        }
        if (j == -1)
            Chat->maxclient = -1; // We no longer have clients.
    }

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

void sendMsgToAllClientsBut(int excluded, char *s, size_t len)
{
    for (int i = 0; i <= Chat->maxclient; i++)
    {
        if (Chat->clients[i] == NULL || excluded == Chat->clients[i]->fd)
            continue;
        /* Important: we don't do ANY BUFFERING. We just use the kernel
         * socket buffers. If the content does not fit, we don't care.
         * This is needed in order to keep this program simple. */
        write(Chat->clients[i]->fd, s, len);
    }
}

int main()
{
    initChat();
    while (1)
    {
        // 文件描述符集合
        fd_set readfds;
        struct timeval tv;
        int retval;
        printf("while running\n");
        // 清空文件描述符集合
        FD_ZERO(&readfds);
        FD_SET(Chat->serversock, &readfds);
        printf("Current clients:%d\n", Chat->numclients);
        for (int i = 0; i <= Chat->maxclient; i++)
        {
            if (Chat->clients[i])
                FD_SET(i, &readfds); // FIXME
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int maxfd = Chat->maxclient;
        if (maxfd < Chat->serversock)
            maxfd = Chat->serversock;
        retval = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("select() error");
            exit(1);
        }
        else if (retval)
        {
            if (FD_ISSET(Chat->serversock, &readfds))
            {
                int fd = acceptClient(Chat->serversock);
                struct client *c = createClient(fd);
                char *welcome_msg =
                    "Welcome to Simple Chat! "
                    "Use /nick <nick> to set your nick.\n";
                // 欢迎语
                write(c->fd, welcome_msg, strlen(welcome_msg));
                printf("Connected client fd=%d\n", fd);
            }

            /* Here for each connected client, check if there are pending
             * data the client sent us. */
            char readbuf[256];
            for (int i = 0; i <= Chat->maxclient; i++)
            {
                if (Chat->clients[i] == NULL)
                    continue;
                if (FD_ISSET(i, &readfds))
                {
                    int nread = read(i, readbuf, sizeof(readbuf) - 1);
                    if (nread <= 0)
                    {
                        /* Error or short read means that the socket
                         * was closed. */
                        printf("Disconnected client fd=%d, nick=%s\n", i, Chat->clients[i]->nick);
                        freeClient(Chat->clients[i]);
                    }
                    else
                    {
                        /* The client sent us a message. We need to
                         * relay this message to all the other clients
                         * in the chat. */
                        struct client *c = Chat->clients[i];
                        readbuf[nread] = 0;

                        if (readbuf[0] == '/')
                        {
                            // 解析输入的昵称
                            char *p;
                            p = strchr(readbuf, '\r');
                            if (p)
                                *p = 0;
                            p = strchr(readbuf, '\n');
                            if (p)
                                *p = 0;
                            char *arg = strchr(readbuf, ' ');
                            if (arg)
                            {
                                *arg = 0;
                                arg++;
                            }
                            if (!strcmp(readbuf, "/nick") && arg)
                            {
                                free(c->nick);
                                int nicklen = strlen(arg);
                                c->nick = chatMalloc(nicklen + 1);
                                memcpy(c->nick, arg, nicklen + 1);
                            }
                            else
                            {
                                char *errmsg = "Unsupported command\n";
                                write(c->fd, errmsg, strlen(errmsg));
                            }
                        }
                        else
                        {
                            // send message to everybody
                            char msg[256];
                            int msglen = snprintf(msg, sizeof(msg), "%s> %s", c->nick, readbuf);
                            printf("%s", msg);
                            sendMsgToAllClientsBut(i, msg, msglen);
                        }
                    }
                }
            }
        }
        else
        {
            // Timeout occurred.
        }
    }

    return 0;
}