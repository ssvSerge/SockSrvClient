#include "SockOsTypes.h"

#if ( PLATFORM == PLATFORM_WINDOWS )

    #include <Winsock2.h>

    #pragma comment( lib, "ws2_32.lib" )

    void sock_init_win ( void ) {
        WSADATA  w_data  = {};
        (void) WSAStartup ( MAKEWORD ( 2, 2 ), &w_data );
    }

    void sock_blocking_win ( sock_t fd ) {
        unsigned long mode = 1;
        ioctlsocket ( fd, FIONBIO, &mode );
    }

    void sock_nonblocking_win ( sock_t fd ) {
        unsigned long mode = 0;
        ioctlsocket ( fd, FIONBIO, &mode );
    }


#elif (PLATFORM == PLATFORM_MAC) || (PLATFORM == PLATFORM_UNIX)

    #include <fcntl.h>

    void sock_init_lin ( void ) {
    }

    void sock_blocking_lin ( sock_t fd ) {
        int flags = fcntl(fd, F_GETFL, 0);
        flags &= ~O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    }

    void sock_nonblocking_lin ( sock_t fd ) {
        int flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    }


#endif
