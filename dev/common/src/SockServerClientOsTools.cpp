#include <HidOsTypes.h>
#include <stdio.h>

#ifndef PLATFORM
    #error "PLATFORM is not defined"
#endif

#if (  (PLATFORM!=PLATFORM_WINDOWS)  &&  (PLATFORM!=PLATFORM_LINUX) )
    #error "PLATFORM is not supported"
#endif

#if ( PLATFORM == PLATFORM_WINDOWS )

    #include <Winsock2.h>
    #include <io.h>
    #pragma comment( lib, "ws2_32.lib" )

#elif (PLATFORM == PLATFORM_LINUX)

    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/tcp.h>

#endif

#include <cerrno>

bool sock_valid ( os_sock_t& sock ) {

    bool ret_val = true;

    if ( sock == static_cast<os_sock_t> (SOCK_INVALID_SOCK) ) {
        ret_val = false;
    }

    return ret_val;
}

int sock_error () {

    #if ( PLATFORM == PLATFORM_WINDOWS )

        int err = WSAGetLastError ();
        if ( err == WSAEWOULDBLOCK ) {
            return EWOULDBLOCK;
        }
        return err;

    #elif (PLATFORM == PLATFORM_LINUX)
        return errno;
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
    #elif (PLATFORM == PLATFORM_LINUX)
        int flags = fcntl(fd, F_GETFL, 0);
        flags &= ~O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    #endif
}

void sock_nonblocking ( os_sock_t fd ) {
    #if ( PLATFORM == PLATFORM_WINDOWS )
        unsigned long mode = 1;
        ioctlsocket ( fd, FIONBIO, &mode );
    #elif (PLATFORM == PLATFORM_LINUX)
        int flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl ( fd, F_SETFL, flags );
    #endif
}

void sock_unlink ( const char* const fname ) {
    #if ( PLATFORM == PLATFORM_WINDOWS )
        _unlink ( fname );
    #elif (PLATFORM == PLATFORM_LINUX)
        unlink ( fname );
    #endif
}

void os_sockclose ( os_sock_t& sock ) {

    if ( sock_valid (sock) ) {

        #if ( PLATFORM == PLATFORM_WINDOWS )
            closesocket ( sock );
        #elif (PLATFORM == PLATFORM_LINUX)
            close ( sock );
        #endif

        sock = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
    }
}

void os_sock_nodelay ( os_sock_t& fd ) {

    #if ( PLATFORM == PLATFORM_WINDOWS )

        DWORD option_value = 1;
        int option_len = sizeof(option_value);
        setsockopt ( fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&option_value), option_len );

    #elif (PLATFORM == PLATFORM_LINUX)

        int option_value = 1;
        socklen_t option_len = sizeof(option_value);
        setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &option_value, option_len);

    #endif
}
