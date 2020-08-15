
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

#if (defined(__linux__) || defined(__APPLE__))
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>
#define OS_POSIX
#endif

#ifdef OS_POSIX

#define SHELL_SERVER_SUN_PATH "/tmp/cmd.server.socket"
#define SHELL_CLIENT_SUN_PATH "/tmp/cmd.client.socket"

#define CLI_PERM        S_IRWXU     /*rwx for user*/
#define RECV_BUF_LEN    (512*1024) /*1M*/
#define SEND_BUF_LEN    1024

#define MAX_CLIENT_NUM  256
#define CLI_PATH_LEN    64
#define EXIT_FAILED     -1

typedef struct CmdHeader{
    int length;     /*shell交互长度:sizeof(CMD_HEADER)+length*/
    int argc;       /*shell交互参数个数*/
    int cmdLen;     /*argv[0]的长度*/
    int argLen;     /*argv[1]的长度*/
} CMD_HEADER;

static int cli_conn(const char *name) {
    int fd = 0;
    int len = 0;
    int err = 0;
    int ret = 0;
    struct sockaddr_un un;

    /*create a UNIX domain stream socket*/
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        printf("cli_conn create UNIX domain socket failed: %s(%d)\n", strerror(errno), errno);
        return EXIT_FAILED;
    }

    /*fill socket address structure with our address*/
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "%s%05d", SHELL_CLIENT_SUN_PATH, getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    /*in cast it already exits*/
    unlink(un.sun_path);
    if (bind(fd, (struct sockaddr*)&un, len) < 0) {
        printf("cli_conn bind UNIX domain socket failed: %s(%d)\n", strerror(errno), errno);
        goto err_exit;
    }

    if (chmod(un.sun_path, CLI_PERM) < 0) {
        printf("cli_conn chmod UNIX domain path failed: %s(%d)", strerror(errno), errno);
        goto err_exit;
    }

    /*fill socket address structure with server's address*/
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(sockaddr_un, sun_path) + strlen(name);

    if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        printf("cli_conn connect UNIX domain socket failed: %s(%d)\n", strerror(errno), errno);
        goto err_exit;
    }
    return fd;
return fd;
    err_exit:
    err = errno;
    close(fd);
    errno = err;
    return EXIT_FAILED;
}

int main(int argc, char *argv[]) {
    /*连接服务器*/
    int clifd = cli_conn(SHELL_SERVER_SUN_PATH);
    if (clifd < 0) {
        return EXIT_FAILED;
    }

    CMD_HEADER header = { 0 };
    char buffer[SEND_BUF_LEN] = { 0 };
    /*构造发送缓冲区*/
    header.argc = argc;
    header.cmdLen = strlen(argv[0])+1;  /*...\0*/
    header.length = sizeof(header) + header.cmdLen;
    if (argc >= 2) {
        header.argLen = strlen(argv[1]);
        header.length = header.length + header.argLen;
        memcpy(buffer + sizeof(header) + header.cmdLen, argv[1], header.argLen);
    }
    memcpy(buffer + sizeof(header), argv[0], header.cmdLen);
    memcpy(buffer, &header, sizeof(header));

    /*发送*/
    int nwrite = 0, nread = 0;
    if ((nwrite = send(clifd, buffer, header.length, 0)) != header.length) {
        printf("send command failed: %s(%d)\n", strerror(errno), errno);
        close(clifd);
        return EXIT_FAILED;
    }

    memset(buffer, 0, sizeof(buffer));
    while ((nread = recv(clifd, buffer, RECV_BUF_LEN, 0)) > 0) {
        buffer[RECV_BUF_LEN - 1] = { 0 };
        printf("%s", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    close(clifd);
    printf("\nno more data received\n");
    return 0;
}

#endif

