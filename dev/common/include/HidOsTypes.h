#ifndef __HIDOSTYPES_H__
#define __HIDOSTYPES_H__

#define  SOCK_INVALID_SOCK      ( -1 )

#define PLATFORM_WINDOWS        (1)
#define PLATFORM_UNIX           (2)

#ifdef _WIN32 
#define PLATFORM                PLATFORM_WINDOWS
#endif


#ifdef _WIN32 

    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #include <afunix.h>

    typedef  int                    sock_len_t;
    typedef  SOCKADDR_IN            sock_addr_t;
    typedef  SOCKET                 os_sock_t;

#else

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>

    typedef socklen_t               sock_len_t;
    typedef struct sockaddr_in      sock_addr_t;
    typedef int                     os_sock_t;

#endif


int  sock_error ( void );
void sock_init ( void );
void sock_blocking ( os_sock_t sock );
void sock_nonblocking ( os_sock_t sock );
void sock_unlink ( const char* const fname );
void os_sockclose ( os_sock_t& sock );


#endif

