#include <stdio.h>
#include <string.h>
#include <sys/types.h>
// #include <sys/socket.h>
// #include <sys/un.h>

#include <winsock2.h>
#include <afunix.h>

#define SUN_LEN(su) (int) ((sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path)))

#define SERVER_PATH     "/tmp/sock_server"
#define BUFFER_LENGTH    250
#define FALSE              0

int main ( void ) {

    int         sd1 = -1;
    int         sd2 = -1;
    int         rc;
    int         length;
    char        buffer[BUFFER_LENGTH];

    struct sockaddr_un  serveraddr;
    WORD                wVersionRequested;
    WSADATA             wsaData;

    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    do {

        sd1 = (int) socket(AF_UNIX, SOCK_STREAM, 0);
        if (sd1 < 0) {
            perror("socket() failed");
            break;
        }

        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sun_family = AF_UNIX;

        strcpy(serveraddr.sun_path, SERVER_PATH);

        rc = bind(sd1, (struct sockaddr*)&serveraddr, SUN_LEN(&serveraddr));
        if (rc < 0) {
            perror("bind() failed");
            break;
        }

        rc = listen(sd1, 10);
        if (rc < 0) {
            perror("listen() failed");
            break;
        }

        printf("Ready for client connect().\n");

        sd2 = accept(sd1, NULL, NULL);
        if (sd2 < 0) {
            perror("accept() failed");
            break;
        }

        length = BUFFER_LENGTH;
        rc = setsockopt(sd2, SOL_SOCKET, SO_RCVLOWAT, (char*)&length, sizeof(length));
        if (rc < 0) {
            perror("setsockopt(SO_RCVLOWAT) failed");
        }

        rc = recv(sd2, buffer, sizeof(buffer), 0);
        if (rc < 0) {
            perror("recv() failed");
            break;
        }

        printf("%d bytes of data were received\n", rc);
        if (rc == 0 || rc < sizeof(buffer)) {
            printf("The client closed the connection before all of the\n");
            printf("data was sent\n");
            break;
        }

        rc = send(sd2, buffer, sizeof(buffer), 0);
        if (rc < 0) {
            perror("send() failed");
            break;
        }

    }   while (FALSE);

    if (sd1 != -1) {
        closesocket(sd1);
    }

    if (sd2 != -1) {
        closesocket(sd2);
    }

    unlink(SERVER_PATH);

    return 0;
}
