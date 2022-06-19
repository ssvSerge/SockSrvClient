#include <HidOsTypes.h>

#ifndef PLATFORM
#error "PLATFORM is not defined"
#endif

#if (  (PLATFORM!=PLATFORM_WINDOWS)  &&  (PLATFORM!=PLATFORM_UNIX) )
#error "PLATFORM is not supported"
#endif

#if ( PLATFORM == PLATFORM_WINDOWS )

    #include <Winsock2.h>
    #include <io.h>
    #pragma comment( lib, "ws2_32.lib" )

#elif (PLATFORM == PLATFORM_UNIX)

    #include <fcntl.h>

#endif

int sock_error () {

    #if ( PLATFORM == PLATFORM_WINDOWS )

        int err = WSAGetLastError ();
        if ( err == WSAEWOULDBLOCK ) {
            return EWOULDBLOCK;
        }
        return err;

    #elif (PLATFORM == PLATFORM_UNIX)
        return errno();
    #endif
}

void sock_init ( void ) {
    #if ( PLATFORM == PLATFORM_WINDOWS )
        WSADATA  w_data  = {};
        (void) WSAStartup ( MAKEWORD ( 2, 2 ), &w_data );
    #endif
}

void sock_blocking ( os_sock_t fd ) {
    #if ( PLATFORM == PLATFORM_WINDOWS )
        unsigned long mode = 0;
        ioctlsocket ( fd, FIONBIO, &mode );
    #elif (PLATFORM == PLATFORM_UNIX)
        int flags = fcntl(fd, F_GETFL, 0);
        flags &= ~O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    #endif
}

void sock_nonblocking ( os_sock_t fd ) {
    #if ( PLATFORM == PLATFORM_WINDOWS )
        unsigned long mode = 1;
        ioctlsocket ( fd, FIONBIO, &mode );
    #elif (PLATFORM == PLATFORM_UNIX)
        int flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    #endif
}

void sock_unlink ( const char* const fname ) {
    #if ( PLATFORM == PLATFORM_WINDOWS )
        _unlink ( fname );
    #elif (PLATFORM == PLATFORM_UNIX)
        unlink ( fname );
    #endif
}

void os_sockclose ( os_sock_t& sock ) {

    if ( sock != SOCK_INVALID_SOCK ) {

        #if ( PLATFORM == PLATFORM_WINDOWS )
            closesocket ( sock );
        #elif (PLATFORM == PLATFORM_UNIX)
            close ( sock );
        #endif

        sock = SOCK_INVALID_SOCK;
    }
}