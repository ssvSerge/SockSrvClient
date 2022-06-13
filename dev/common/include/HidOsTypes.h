#ifndef __HIDOSTYPES_H__
#define __HIDOSTYPES_H__

#ifdef _WIN32 

    #include <WinSock2.h>
    #include <afunix.h>

    #define  SOCK_INVALID_SOCK      ( -1 )

    typedef  int                    sock_len_t;
    typedef  SOCKADDR_IN            sock_addr_t;
    typedef  SOCKET                 os_sock_t;

    void sock_init_win ( void );
    void sock_blocking_win ( os_sock_t sock );
    void sock_nonblocking_win ( os_sock_t sock );
    void sock_unlink_win ( const char* const fname );

    #define os_sockclose            closesocket
    #define os_sock_init            sock_init_win
    #define os_sock_unlink          sock_unlink_win
    #define sock_to_blocked         sock_blocking_win
    #define sock_to_nonblocked      sock_nonblocking_win

#else

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>

    #define  SOCK_INVALID_SOCK      ( -1 )

    typedef socklen_t               sock_len_t;
    typedef struct sockaddr_in      sock_addr_t;
    typedef int                     os_sock_t;

    void sock_init_lin ( void );
    void sock_blocking_lin ( sock_t sock );
    void sock_nonblocking_lin ( sock_t sock );
    void sock_unlink_lin ( const char* const fname );

    #define os_sockclose            close
    #define os_sock_init            sock_init_lin
    #define os_sock_unlink          sock_unlink_lin
    #define sock_to_blocked         sock_blocking_lin
    #define sock_to_nonblocked      sock_nonblocking_lin


#endif


#endif

