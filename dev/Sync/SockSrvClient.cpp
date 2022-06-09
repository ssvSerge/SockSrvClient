#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>

#include "SockServerClient.h"
#include "Serializer.h"

using namespace std::chrono_literals;

// [0] 4 bytes       magic;
// [1] 4 bytes        type;
// [2] 4 bytes         len;
// [3] 4 bytes        code;
// [4] 4 bytes  timeout_hi;
// [5] 4 bytes  timeout_lo;
// [6] 4 bytes         crc;

constexpr int HDR_LENGTH = ( 7 * sizeof(uint32_t) );
constexpr int HDR_MAGIC  = 0x81726354U;
constexpr int HDR_CRC    = 0xFFFFFFFFU;

//---------------------------------------------------------------------------//

static int open_socket ( bool bind_to, const char* const portStr, conn_type_t type ) {

    int ret_sock = -1;

    #if ( PLATFORM == PLATFORM_WINDOWS )
    {   
        int                 io_res      = false;
        sockaddr_in         net_sock    = {};
        SOCKADDR_UN         local_sock  = {};
        struct sockaddr*    addr_ptr    = nullptr;;
        size_t              addr_size   = 0;

        ret_sock = socket ( AF_INET, SOCK_STREAM, 0 );
        if ( ret_sock == SOCK_INVALID_SOCK ) {
            return false;
        }

        if ( bind_to ) {

            if ( type == conn_type_t::CONN_TYPE_SOCK ) {

                int port = atoi ( portStr );

                net_sock.sin_port = htons ( port );
                net_sock.sin_family = AF_INET;

                addr_size = sizeof ( net_sock );
                addr_ptr = reinterpret_cast<struct sockaddr*>(&net_sock);

            } else {

                local_sock.sun_family = AF_UNIX;
                strncpy ( local_sock.sun_path, portStr, sizeof ( local_sock.sun_path ) - 1 );

                addr_size = sizeof ( local_sock );
                addr_ptr  = reinterpret_cast<struct sockaddr*>(&local_sock);

            }

            io_res = bind ( ret_sock, addr_ptr, addr_size );
            if ( io_res == SOCKET_ERROR ) {
                return false;
            }

        }

    }
    #endif

    return ret_sock;
}

//---------------------------------------------------------------------------//

msg_header_t::msg_header_t () {

}

msg_header_t::~msg_header_t () {

}
        
void msg_header_t::format ( frame_type_t type, uint32_t err_code, uint64_t timeout, uint32_t len ) {

    uint8_t*    hdr_basic_;
    uint32_t*   ptr;
    uint32_t    timeout_hi;
    uint32_t    timeout_lo;

    timeout_hi = static_cast<uint32_t> ( (timeout >> 32) & 0xFFFFFFFFU);
    timeout_lo = static_cast<uint32_t> ( timeout & 0xFFFFFFFFU );

    m_hdr.resize (HDR_LENGTH);

    hdr_basic_  = m_hdr.data();
    ptr         = reinterpret_cast<uint32_t*> (hdr_basic_);

    ptr [0] = HDR_MAGIC;
    ptr [1] = static_cast<uint32_t> (type);
    ptr [2] = len;
    ptr [3] = err_code;
    ptr [4] = timeout_hi;
    ptr [5] = timeout_lo;
    ptr [6] = calc_crc (ptr);

    return;
}

bool msg_header_t::valid ( sock_frame_t& frame ) {

    uint8_t*    hdr_basic_;
    uint32_t*   hdr_data;

    if ( frame.size () < HDR_LENGTH ) {
        return false;
    }

    hdr_basic_  = frame.data();
    hdr_data    = reinterpret_cast<uint32_t*> (hdr_basic_);

    if ( hdr_data [0] != HDR_MAGIC ) {
        return false;
    }

    frame_type_t frame_type = static_cast<frame_type_t> (hdr_data [1]);

    switch( frame_type ) {
        case frame_type_t::FRAME_TYPE_REQUEST:
        case frame_type_t::FRAME_TYPE_RESPONSE:
        case frame_type_t::FRAME_TYPE_TIMEOUT:
            break;
        default:
            return false;
    }

    uint32_t frame_crc;
    frame_crc = HDR_CRC - hdr_data [0] - hdr_data [1] - hdr_data [2] - hdr_data [3];

    if ( frame_crc != hdr_data [3] ) {
        return false;
    }

    return true;
}

uint32_t msg_header_t::calc_crc ( const void* const ptr ) {

    uint32_t ret_val = 0;

    const uint32_t* const data = static_cast<const uint32_t*> (ptr);

    ret_val  = HDR_CRC;   // Start
    ret_val -= data[0];   // Magic
    ret_val -= data[1];   // Type
    ret_val -= data[2];   // len
    ret_val -= data[3];   // code
    ret_val -= data[4];   // timeout / hi
    ret_val -= data[5];   // timeout / lo

    return ret_val;
}

//---------------------------------------------------------------------------//

sock_transaction_t::sock_transaction_t () {

    m_closed = false;
    m_sock  =  SOCK_INVALID_SOCK;
    m_idx   =  SOCK_INVALID_IDX;
}

sock_transaction_t::~sock_transaction_t () {

}

sock_transaction_t::sock_transaction_t ( const sock_transaction_t& ref ) {
    move_to_me (ref);
};

void sock_transaction_t::move_to_me ( const sock_transaction_t& ref ) {

    this->m_start_time      = ref.m_start_time;
    this->m_commit_time     = ref.m_commit_time;
    this->m_expiration_time = ref.m_expiration_time;
    this->m_idx             = ref.m_idx;
    this->m_sock            = ref.m_sock;

    this->in_data  = std::move ( const_cast<sock_transaction_t&> (ref).in_data  );
    this->out_data = std::move ( const_cast<sock_transaction_t&> (ref).out_data );

    const_cast<sock_transaction_t&> (ref).m_idx  = SOCK_INVALID_IDX;
    const_cast<sock_transaction_t&> (ref).m_sock = SOCK_INVALID_SOCK;
}

void sock_transaction_t::set_timeout ( sock_duration_ms_t timeout_ms ) {

    m_start_time = std::chrono::steady_clock::now ();
    m_expiration_time = m_start_time + timeout_ms;

}

void sock_transaction_t::checkpoint_set ( sock_checkpoint_t point_type ) {

    sock_timepoint_t ref = std::chrono::steady_clock::now ();

    switch ( point_type ) {
        case sock_checkpoint_t::TIMEPOINT_RCV_HDR:
            m_rcv_hdr = ref;
            break;
        case sock_checkpoint_t::TIMEPOINT_RCV_PAYLOAD:
            m_rcv_payload = ref;
            break;
        case sock_checkpoint_t::TIMEPOINT_SNT_HDR:
            m_snt_hdr = ref;
            break;
        case sock_checkpoint_t::TIMEPOINT_SNT_PAYLOAD:
            m_snt_payload = ref;
            break;
    }
}

bool sock_transaction_t::is_expired () {

    sock_timepoint_t ref = std::chrono::steady_clock::now ();

    if ( ref > m_expiration_time ) {
        return true;
    }

    return false;
}

void sock_transaction_t::close () {

    if ( ! m_closed ) {
        m_closed = true;
        m_commit_time = std::chrono::steady_clock::now ();
    }
}

//---------------------------------------------------------------------------//

SocketServer::SocketServer () {

    m_err_cnt       =  0;
    m_server_sock   = -1;
    m_id            =  0;
    m_handler       = nullptr;
    m_stop          = false;

    sock_init ();
}

SocketServer::~SocketServer () {

    Stop ();
}

bool SocketServer::Start ( const char* const portStr, conn_type_t type ) {

    m_server_sock = open_socket ( true, portStr, type );

    if ( m_server_sock == INVALID_SOCKET) {
        return false;
    }

    m_server = std::async ( std::launch::async, &SocketServer::Service, this, m_server_sock );

    return true;
}

void SocketServer::Stop ( void ) {

    m_stop = true;

    if ( m_server.valid () ) {
        m_server.get ();
    }

    for ( auto &client : m_clients ) {
        if ( client.valid () ) {
            client.get();
        }
    }

    m_clients.clear ();

    sock_close ( m_server_sock );
}

bool SocketServer::SendFrame ( sock_t sock, const sock_frame_t& frame ) {

    bool   ret_val   = false;
    size_t tx_cnt    = 0;
    size_t data_part = 0;
    size_t io_res    = 0;

    const uint8_t* src_ptr = frame.data();

    if ( frame.size () == 0 ) {
        return true;
    }

    sock_to_blocked (sock);

    while ( tx_cnt < frame.size () ) {

        if ( m_stop ) {
            break;
        }

        data_part = frame.size () - tx_cnt;

        if ( data_part > 65536 ) {
            data_part = 65536;
        }

        io_res = send ( sock, (char*)(src_ptr+tx_cnt), data_part, 0);

        if ( io_res < 0 ) { 
            break;
        }

        tx_cnt += io_res;

    }

    if ( tx_cnt != frame.size () ) {
        return false;
    }

    return true;
}

bool SocketServer::GetTimeout ( bool expiration_valid, sock_timepoint_t exp_time, struct timeval& tv ) {

    if ( ! expiration_valid ) {
        tv.tv_sec  = 10;
        tv.tv_usec = 0;
        return true;
    }

    sock_timepoint_t my_time = std::chrono::steady_clock::now();

    if ( my_time > exp_time ) {
        return false;
    }

    std::chrono::steady_clock::duration diff = my_time - exp_time;
    auto milliseconds = std::chrono::duration_cast< std::chrono::milliseconds  >( diff ).count();

    tv.tv_sec  = static_cast<long> (milliseconds / 1000000);
    tv.tv_usec = static_cast<long> (milliseconds % 1000000);

    return true;
}

bool SocketServer::RecvFrame ( sock_t sock, sock_frame_t& frame, sock_timepoint_t exp_time, bool apply_expiration) {

    struct timeval tv       = {};
    fd_set      fds         = {};
    int         io_res      = 0;
    uint8_t*    dst         = nullptr;
    uint32_t    data_part   = 0;

    sock_to_nonblocked (sock);

    for ( size_t rx_cnt = 0; rx_cnt < frame.size (); ) {

        if ( m_stop ) {
            // Stop requested.
            break;
        }

        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        // Load tmieout. 
        if ( ! GetTimeout (apply_expiration, exp_time, tv ) ) {
            // Expired.
            break;
        }

        io_res = select ( sock + 1, NULL, &fds, NULL, &tv );

        // Ignore "non-error" states.
        if ( io_res == -1 ) {
            if ( errno == EAGAIN ) {
                io_res = 0;
            } else 
            if( errno == EWOULDBLOCK ) {
                io_res = 0;
            }
        }

        if ( io_res == 0 ) {
            // Timeout happened.
            continue;
        }

        if ( io_res < 0 ) {
            // Error state.
            break;
        }

        dst         = frame.data()  + rx_cnt;
        data_part   = frame.size () - rx_cnt;
        io_res      = recv (sock, (char*)dst, data_part, 0 );

        if ( io_res < 0 ) {
            // Read error. 
            break;
        }

        if ( io_res == 0 ) {
            // Read after close. Connection closed by remote side.
            break;
        }

        rx_cnt += io_res;
    }

    return true;
}

void SocketServer::SetHandler ( ev_handler_t handler ) {

    m_handler = handler;
}

bool SocketServer::ConnProcessNew ( sock_t server_socket ) {

    int             client_sock;
    fd_set          readfds;
    struct timeval  timeout;

    FD_ZERO(&readfds);
    FD_SET(server_socket, &readfds);

    // timeout.tv_sec  = 0;        // 
    // timeout.tv_usec = 200000;   // 5 times per second

    timeout.tv_sec  =  5;          // once per 20 seconds.
    timeout.tv_usec =  0;          // 

    int sel_res = select(server_socket + 1 , &readfds , NULL , NULL , &timeout);

    if ( sel_res == -1 ) {
        if ( errno == EWOULDBLOCK ) {
            sel_res = 0;
        } else
        if ( errno == EAGAIN ) {
            sel_res = 0;
        }
    }

    if ( sel_res == 0 ) {
        m_err_cnt = 0;
        return true;
    }

    if ( sel_res > 0 ) {

        client_sock = accept ( server_socket, NULL, NULL );

        if ( client_sock != -1 ) {

            m_err_cnt = 0;
            sock_thread_t client_shell;
            client_shell = std::async ( std::launch::async, &SocketServer::Shell, this, client_sock );
            m_clients.emplace_back ( std::move(client_shell) );
            return true;
        }

    }

    m_err_cnt++;
    if ( m_err_cnt >= SOCK_SRV_MAX_ERRORS_CNT ) {
        return false;
    }

    return true;
}

bool SocketServer::ConnMoveToExpired () {

    bool acquired;
    
    acquired = m_controller.try_lock ();

    if ( ! acquired ) {
        return true;
    }

    try {

        for ( auto pos = m_pending_list.begin (); pos != m_pending_list.end (); ) {
            if ( pos->is_expired () ) {
                m_rejected_list.push_back ( *pos );
                pos = m_pending_list.erase ( pos );
            } else {
                pos++;

            }
        }

    } catch( ... ) {
    }

    m_controller.unlock ();

    return true;
}

bool SocketServer::ConnProcessExpired () {

    bool                acquired;
    sock_transaction_t  tr;
    msg_header_t        hdr;

    while ( true ) {

        if ( ! m_controller.try_lock () ) {
            break;
        }

        acquired = false;

        try {
            if ( m_rejected_list.size () > 0 ) {
                tr.move_to_me ( m_rejected_list.front () );
                m_rejected_list.pop_front ();
                acquired = true;
            }
        } catch ( ... ) {
        }

        m_controller.unlock ();

        if ( ! acquired ) {
            break;
        }

        tr.out_data.clear ();

        hdr.format (frame_type_t::FRAME_TYPE_TIMEOUT, 0, 0, 0 );

        SendFrame ( tr.m_sock, hdr.m_hdr );

        sock_close (tr.m_sock);
    }

    return true;
}

bool SocketServer::Service ( sock_t server_socket ) {

    bool io_res;

    while ( ! m_stop ) {

        io_res = ConnProcessNew (server_socket);
        if ( ! io_res ) {
            break;
        }

        ConnMoveToExpired ();
        ConnProcessExpired ();
    }

    sock_close (server_socket);
    server_socket = SOCK_INVALID_SOCK;

    return true;
}

bool SocketServer::ConnRecvHdr ( sock_t client_socket ) {

    return true;
}

bool SocketServer::ConnRecvPayload ( sock_t client_socket ) {

    return true;
}

bool SocketServer::ConnSendHdr ( sock_t client_socket ) {

    return true;
}

bool SocketServer::ConnSendPayload ( sock_t client_socket ) {

    return true;
}

bool SocketServer::Shell ( sock_t client_socket ) {

    while ( ! m_stop ) {

        std::this_thread::sleep_for ( 1000ms );

        ConnRecvHdr      ( client_socket );
        ConnRecvPayload  ( client_socket );
        ConnSendHdr      ( client_socket );
        ConnSendPayload  ( client_socket );

    }

    return true;
}

//---------------------------------------------------------------------------//

SocketClient::SocketClient () {

    m_client_sock = -1;
    m_type = conn_type_t::CONN_TYPE_UNKNOWN;
    sock_init ();
}

SocketClient::~SocketClient () {

}

bool SocketClient::Connect ( const char* const portStr, conn_type_t type ) {

    m_port = portStr;
    m_type = type;

    m_client_sock = open_socket ( true, portStr, type );

    if ( m_client_sock == SOCK_INVALID_SOCK) {
        return false;
    }

    return true;
}

bool SocketClient::Transaction ( sock_duration_ms_t duration, const sock_frame_t& out_fame, sock_frame_t& in_frame, frame_type_t& status ) {

    auto diff     = std::chrono::milliseconds( 16 );
    auto ts_start = std::chrono::high_resolution_clock::now ();
    auto ts_end   = ts_start + diff;

    return true;
}

//---------------------------------------------------------------------------//
