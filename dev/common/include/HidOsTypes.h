#ifndef __HIDOSTYPES_H__
#define __HIDOSTYPES_H__

#define  SOCK_INVALID_SOCK      ( -1 )

#define PLATFORM_WINDOWS        (1)
#define PLATFORM_LINUX          (2)

#ifdef _WIN32 
#define PLATFORM                PLATFORM_WINDOWS
#endif

#ifdef __linux__
#define PLATFORM                PLATFORM_LINUX
#endif



#ifdef _WIN32 

    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #include <afunix.h>

    typedef  int                    sock_len_t;
    typedef  SOCKADDR_IN            sock_addr_t;
    typedef  SOCKET                 os_sock_t;

    #define  SUN_LEN(p)  ((size_t) (( (struct sockaddr_un*) NULL)->sun_path) + strlen ((p)->sun_path))


#else

    #include <fcntl.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <arpa/inet.h>



    // typedef socklen_t               sock_len_t;
    // typedef struct sockaddr_in      sock_addr_t;
    typedef int                     os_sock_t;

#endif


int  sock_error ( void );
void sock_init ( void );
void sock_blocking ( os_sock_t sock );
void sock_nonblocking ( os_sock_t sock );
void sock_unlink ( const char* const fname );
void os_sockclose ( os_sock_t& sock );
void os_sock_nodelay ( os_sock_t& sock );
bool sock_valid ( os_sock_t& sock );
os_sock_t os_socket ( int mode, int type, int proto );


#endif

