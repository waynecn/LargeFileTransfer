#include <iostream>
#include <stdio.h>
#include <WinSock2.h>


using namespace std;

//缓存大小设置不能超过2M
#define BUFF_SIZE (1024 * 1024)
#define FILE_NAME_LENGTH 1024


int s;                     /* socket for accepting connections    */
int ns;                    /* socket connected to client          */

int exitFunc() {
    closesocket(s);
    closesocket(ns);
}

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

char *getFileName(char *filePath) {
    bool bFound = false;
    char *buff = new char[1024];
    memset(buff, 0, 1024);
    while (!bFound) {
        int lastIndex = 0;
        for (int i = 0; i < strlen(filePath); ++i) {
            if (filePath[i] == '\\' || filePath[i] == '/') {
                lastIndex = i;
            }
        }
        for (int i = lastIndex + 1; i < strlen(filePath); ++i) {
            buff[i - lastIndex - 1] = filePath[i];
        }
        bFound = true;
    }
    return buff;
}

int main(int argc, char **argv)
{
    _onexit(exitFunc);
    unsigned short port;       /* port server binds to                */
    char buff[BUFF_SIZE];              /* buffer for sending & receiving data */
    struct sockaddr_in client; /* client address information          */
    struct sockaddr_in server; /* server address information          */
    int namelen;               /* length of client name               */
    char *filePath = new char[FILE_NAME_LENGTH];

    //检查是否传入端口参数
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    //第一个参数是端口号
    port = (unsigned short) atoi(argv[1]);
    //如果有第二个参数 第二个参数需要是文件的详细路径 否则需要自己指定路径
    if (argc > 2) {
        filePath = argv[2];
        printf("filePath from arg:%s\n", filePath);
    } else {
        //char *filePath = "D:\\Download\\qt-opensource-windows-x86-5.12.5.exe";
        //char *filePath = "D:\\Download\\ideaIC-2019.3.3.exe";
        filePath = "D:\\Download\\settings.xml";
    }

    off64_t fileSize = getFileSize(filePath);
    printf("fileSize:%lld\n", fileSize);
    char *fileName = getFileName(filePath);
    printf("fileName:%s\n", fileName);

    WSADATA wsadata;
    WSAStartup(0x202, &wsadata);

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
        //再将文件名发送给客户端
        printf("sizeof:%d strlen:%d\n", sizeof(fileName), strlen(fileName));
        if (send(ns, fileName, strlen(fileName), 0) < 0) {
            printf("send fileName to client error\n");
            exit(7);
        }
        while (sendSize < fileSize) {
            memset(buff, 0, 1024 * 1024);
            size_t iread = fread(buff, sizeof(char), BUFF_SIZE, f);
            printf("iread:%d\n", iread);
            if (iread < 0) {
                printf("fread error\n");
                fclose(f);
                break;
            }
            int iSend = send(ns, buff, iread, 0);
            if (iSend < 0) {
                printf("send error\n");
                fclose(f);
                break;
            }
            sendSize += iSend;
            printf("fileSize:%lld iSend:%d sendSize:%lld\n", fileSize, iSend, sendSize);
            fseeko64(f, sendSize, SEEK_SET);
        }
        fclose(f);
    }

    printf("Server ended successfully\n");
    exit(0);
}
