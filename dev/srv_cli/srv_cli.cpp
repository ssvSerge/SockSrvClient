/* Server program example for IPv4 */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define DEFAULT_PORT 2007
// default TCP socket type
#define DEFAULT_PROTO SOCK_STREAM

void Usage ( char* progname )
{
    fprintf ( stderr, "Usage: %s -p [protocol] -e [port_num] -i [ip_address]\n", progname );
    fprintf ( stderr, "Where:\n\t- protocol is one of TCP or UDP\n" );
    fprintf ( stderr, "\t- port_num is the port to listen on\n" );
    fprintf ( stderr, "\t- ip_address is the ip address (in dotted\n" );
    fprintf ( stderr, "\t  decimal notation) to bind to. But it is not useful here...\n" );
    fprintf ( stderr, "\t- Hit Ctrl-C to terminate server program...\n" );
    fprintf ( stderr, "\t- The defaults are TCP, 2007 and INADDR_ANY.\n" );
    WSACleanup ();
    exit ( 1 );
}

int main ( int argc, char** argv ) {
    char Buffer [128];
    char* ip_address = NULL;
    unsigned short port = 4433;
    int retval;
    int fromlen;
    int io;
    int socket_type = DEFAULT_PROTO;
    struct sockaddr_in local, from;
    WSADATA wsaData;
    SOCKET listen_socket, msgsock;

    (void) WSAStartup ( 0x202, &wsaData );

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons ( port );

    listen_socket = socket ( AF_INET, socket_type, 0 );
    io = bind ( listen_socket, (struct sockaddr*) &local, sizeof ( local ) );

    io = listen ( listen_socket, 5 );

    while ( 1 ) {

        fromlen = sizeof ( from );
        msgsock = accept ( listen_socket, (struct sockaddr*) &from, &fromlen );
        retval = recv ( msgsock, Buffer, sizeof ( Buffer ), 0 );
    }

    return 0;

}
