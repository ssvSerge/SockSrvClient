#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <string.h>
// #include <sys/socket.h>
// #include <sys/un.h>

#include <winsock2.h>
#include <afunix.h>        
#include <string.h>

#define SUN_LEN(su) (int) ((sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path)))

#define SERVER_PATH     "/tmp/sock_server"
#define BUFFER_LENGTH    250
#define FALSE              0

int main ( void ) {

    int     sd = -1;

    int     rc;
    int     bytesReceived;
    char    buffer[BUFFER_LENGTH];

    struct sockaddr_un  serveraddr;
    WORD                wVersionRequested;
    WSADATA             wsaData;

    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    do {

        sd = (int) socket(AF_UNIX, SOCK_STREAM, 0); 
        if (sd < 0) {
            perror("socket() failed");
            break;
        }

        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sun_family = AF_UNIX;
        strcpy(serveraddr.sun_path, SERVER_PATH);

        rc = connect ( sd, (struct sockaddr*)&serveraddr, SUN_LEN(&serveraddr) ); 
        if (rc < 0) {
            perror("connect() failed");
            break;
        }

        memset(buffer, 'a', sizeof(buffer));
        rc = send(sd, buffer, sizeof(buffer), 0);
        if (rc < 0) {
            perror("send() failed");
            break;
        }

        bytesReceived = 0;
        while (bytesReceived < BUFFER_LENGTH) {

            rc = recv(sd, &buffer[bytesReceived], BUFFER_LENGTH - bytesReceived, 0);

            if (rc < 0) {
                perror("recv() failed");
                break;
            } else 
            if (rc == 0) {
                printf("The server closed the connection\n");
                break;
            }

            bytesReceived += rc;
        }

    }   while (FALSE);

    if (sd != -1) {
        closesocket(sd);
    }

    return 0;
}