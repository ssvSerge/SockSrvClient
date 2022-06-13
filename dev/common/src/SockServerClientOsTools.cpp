#include <HidOsTypes.h>


#if ( PLATFORM == PLATFORM_WINDOWS )

    #include <Winsock2.h>
    #include <io.h>

    #pragma comment( lib, "ws2_32.lib" )

    void sock_init_win ( void ) {
        WSADATA  w_data  = {};
        (void) WSAStartup ( MAKEWORD ( 2, 2 ), &w_data );
    }

    void sock_blocking_win ( os_sock_t fd ) {
        unsigned long mode = 1;
        ioctlsocket ( fd, FIONBIO, &mode );
    }

    void sock_nonblocking_win ( os_sock_t fd ) {
        unsigned long mode = 0;
        ioctlsocket ( fd, FIONBIO, &mode );
    }

    void sock_unlink_win ( const char* const fname ) {
        _unlink ( fname );
    }



#elif (PLATFORM == PLATFORM_MAC) || (PLATFORM == PLATFORM_UNIX)

    #include <fcntl.h>

    void sock_init_lin ( void ) {
    }

    void sock_blocking_lin ( os_sock_t fd ) {
        int flags = fcntl(fd, F_GETFL, 0);
        flags &= ~O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    }

    void sock_nonblocking_lin ( os_sock_t fd ) {
        int flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    }


#endif
