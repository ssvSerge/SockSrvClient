#ifndef __SOCKOSTYPES_H__
#define __SOCKOSTYPES_H__

    #define PLATFORM_WINDOWS  1
    #define PLATFORM_MAC      2
    #define PLATFORM_UNIX     3

    #if defined(_WIN32)
        #define PLATFORM        PLATFORM_WINDOWS
    #elif defined(__APPLE__)
        #define PLATFORM        PLATFORM_MAC
    #else
        #define PLATFORM        PLATFORM_UNIX
    #endif

    #if ( PLATFORM == PLATFORM_WINDOWS )

        #include <winsock2.h>
        #include <afunix.h>

        typedef int                     sock_len_t;
        typedef SOCKADDR_IN             sock_addr_t;
        typedef SOCKET                  sock_t;

        void sock_init_win              ( void );
        void sock_blocking_win          ( sock_t sock );
        void sock_nonblocking_win       ( sock_t sock );

        #define sock_close              closesocket
        #define sock_to_blocked         sock_blocking_win
        #define sock_to_nonblocked      sock_nonblocking_win
        #define sock_init               sock_init_win
        #define SOCK_INVALID_SOCK       ( -1 )
        #define SOCK_INVALID_IDX        ( -1 )


    #elif (PLATFORM == PLATFORM_MAC) || (PLATFORM == PLATFORM_UNIX)

        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <fcntl.h>

        typedef socklen_t               sock_len_t;
        typedef struct sockaddr_in      sock_addr_t;
        typedef int                     sock_t;

        void sock_init_lin              ( void );
        void sock_blocking_lin          ( sock_t sock );
        void sock_nonblocking_lin       ( sock_t sock );

        #define sock_close              close
        #define sock_to_blocked         sock_blocking_lin
        #define sock_to_nonblocked      sock_nonblocking_lin
        #define sock_init               sock_init_lin
        #define SOCK_INVALID_SOCK       ( -1 )
        #define SOCK_INVALID_IDX        ( -1 )

    #endif

#endif
