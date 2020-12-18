#include <iostream>
#include <stdio.h>
#include <WinSock2.h>

//缓存大小设置不能超过2M
#define BUFF_SIZE (1024 * 1024)

using namespace std;


off64_t getFileSize(char *filePath) {
    FILE *f;
    f = fopen(filePath, "rb");
    if (NULL == f) {
        printf("getFileSize fopen error\n");
        return -1;
    }

    if (0 != fseeko64(f, 0, SEEK_END)) {
        printf("getFileSize fseek error\n");
        return -1;
    }

    off64_t fileSize = ftello64(f);
    if (fileSize < 0) {
        printf("ftell error\n");
    }
    printf("fileSize:%lld\n", fileSize);
    fclose(f);
    return fileSize;
}

int main(int argc, char **argv)
{
    char *filePath = "D:\\Download\\qt-opensource-windows-x86-5.12.5.exe";
    //char *filePath = "D:\\Download\\ideaIC-2019.3.3.exe";
    off64_t fileSize = getFileSize(filePath);
    printf("fileSize:%lld\n", fileSize);

    WSADATA wsadata;
    WSAStartup(0x202, &wsadata);

    unsigned short port;       /* port server binds to                */
    char buff[BUFF_SIZE];              /* buffer for sending & receiving data */
    struct sockaddr_in client; /* client address information          */
    struct sockaddr_in server; /* server address information          */
    int s;                     /* socket for accepting connections    */
    int ns;                    /* socket connected to client          */
    int namelen;               /* length of client name               */

    //检查是否传入端口参数
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    //第一个参数是端口号
    port = (unsigned short) atoi(argv[1]);

    //创建socket服务
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket error\n");
        exit(2);
    }

    //socket和服务地址绑定
    server.sin_family = AF_INET;
    server.sin_port   = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("bind error\n");
        exit(3);
    }

    //监听服务，只允许一个客户端连接
    if (listen(s, 1) != 0)
    {
        printf("listen error\n");
        exit(4);
    }

    //等待连接
    namelen = sizeof(client);
    while (true) {
        //循环 一直等待客户端的连接
        if ((ns = accept(s, (struct sockaddr *)&client, &namelen)) == -1)
        {
            printf("accept error\n");
            exit(5);
        }

        //有客户端连接过来之后 将指定文件发送给客户端
        FILE *f;
        f = fopen(filePath, "rb");
        if (f == NULL) {
            printf("file:%s doesn't exist\n", filePath);
            exit(6);
        }

        off64_t sendSize = 0;
        //先将文件大小的数据发送给客户端
        lltoa(fileSize, buff, 10);
        if (send(ns, buff, sizeof(buff), 0) < 0) {
            printf("send fileSize to client error\n");
            exit(7);
        }
        while (sendSize < fileSize) {
            memset(buff, 0, 1024 * 1024);
            size_t iread = fread(buff, sizeof(char), BUFF_SIZE, f);
            printf("iread:%d\n", iread);
            if (iread < 0) {
                printf("fread error\n");
                fclose(f);
                exit(7);
            }
            int iSend = send(ns, buff, iread, 0);
            if (iSend < 0) {
                printf("send error\n");
                fclose(f);
                exit(7);
            }
            sendSize += iSend;
            printf("fileSize:%lld iSend:%d sendSize:%lld\n", fileSize, iSend, sendSize);
            fseeko64(f, sendSize, SEEK_SET);
        }
        fclose(f);
    }

    //close(ns);
    //close(s);

    printf("Server ended successfully\n");
    exit(0);
}
