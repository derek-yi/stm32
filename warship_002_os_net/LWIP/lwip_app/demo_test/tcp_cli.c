
#include <Winsock2.h>
#include <stdio.h>

 
#pragma comment(lib, "ws2_32.lib") 

void main()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;


    wVersionRequested = MAKEWORD( 1, 1 );
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 ) {                           
        return;
    }

    if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 ) {
        WSACleanup();
        return;
    }

    SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    SOCKADDR_IN sockAddr;
    sockAddr.sin_addr.S_un.S_addr = inet_addr("192.168.0.107");
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(8088);

    while (connect(sock, (SOCKADDR*)& sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        printf("连接失败\n");
        Sleep(1000);
    }
    printf("成功连接到服务器！\n");

    char recvBuf[256];
    char sendBuf[256];
    char tempBuf[300];
    int len = sizeof(SOCKADDR);

    while(1){
        printf("please input:");
        gets(sendBuf);
        if(sendBuf[0] == 'q'){
            printf("Chat end!\n");
            break;
        }

        send(sock, sendBuf, strlen(sendBuf)+1, NULL);
        recv(sock, recvBuf, 100, 0, (SOCKADDR*)&sockAddr, &len);
        sprintf(tempBuf, "%s say: %s", inet_ntoa(sockAddr.sin_addr), recvBuf);
        printf("%s \n",tempBuf);
    }

    closesocket(sock);
    WSACleanup();
}

