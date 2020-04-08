
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

    if ( LOBYTE( wsaData.wVersion ) != 1 ||
            HIBYTE( wsaData.wVersion ) != 1 ) {
        WSACleanup();
        return;
    }

    SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, 0);
    
    SOCKADDR_IN addrClient;
    addrClient.sin_addr.S_un.S_addr = inet_addr("192.168.0.107");
    addrClient.sin_family = AF_INET;
    addrClient.sin_port = htons(8089);

    char recvBuf[128];
    char sendBuf[128];
    char tempBuf[200];
    int len = sizeof(SOCKADDR);

    while(1){
        printf("please input:");
        gets(sendBuf);
        if(sendBuf[0] == 'q'){
            printf("Chat end!\n");
            break;
        }

        sendto(sockClient, sendBuf, strlen(sendBuf)+1, 0, (SOCKADDR*)&addrClient, len);
        recvfrom(sockClient, recvBuf, 100, 0, (SOCKADDR*)&addrClient, &len);

        sprintf(tempBuf, "%s say: %s", inet_ntoa(addrClient.sin_addr), recvBuf);
        printf("%s \n",tempBuf);
    }

    closesocket(sockClient);
    WSACleanup();
}

