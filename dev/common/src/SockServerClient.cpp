#include <SockServerClient.h>
#include <StreamPrefix.h>


#define  SUN_LEN(p)  ((size_t) (( (struct sockaddr_un*) NULL)->sun_path) + strlen ((p)->sun_path))


namespace hid {
namespace socket {

//---------------------------------------------------------------------------//

constexpr size_t TRANSACTION_INVALID_IDX =  0;
constexpr int    SOCK_MAX_ERRORS_CNT     = 15;

//---------------------------------------------------------------------------//

static void dummy_ev_handler ( const hid::types::storage_t& in_data, hid::types::storage_t& out_data, int& error_code ) {

    out_data.resize(0);
    error_code = 100;
}

//---------------------------------------------------------------------------//

static bool socket_open ( conn_type_t conn_type, os_sock_t& sock ) {

    bool ret_val = false;
    int  af_mode = AF_UNSPEC;

    if ( conn_type == conn_type_t::CONN_TYPE_SOCK ) {
        af_mode = AF_INET;
    } else
    if ( conn_type == conn_type_t::CONN_TYPE_FILE ) {
        af_mode = AF_UNIX;
    }
        
    sock = ::socket ( af_mode, SOCK_STREAM, 0 );
    if ( sock == SOCK_INVALID_SOCK ) {
        // TTRACE ( failed to open new socket );
    }

    return ret_val;
}

static void socket_close ( os_sock_t& sock ) {

    if ( sock != SOCK_INVALID_SOCK ) {
        os_sockclose ( sock );
    }
    sock = SOCK_INVALID_SOCK;
}

static bool socket_bind ( os_sock_t sock, conn_type_t conn_type, const char* const port_str ) {

    bool ret_val = false;

    #if ( PLATFORM == PLATFORM_WINDOWS )
    {
        sockaddr_in         local_sock  = {};
        struct sockaddr_un  local_file  = {};
        struct sockaddr*    addr_ptr    = nullptr;
        int                 addr_size   = 0;

        int io_res  = 0;

        if ( conn_type == conn_type_t::CONN_TYPE_SOCK ) {

            int port = atoi ( port_str );

            local_sock.sin_family = AF_INET;
            local_sock.sin_port = htons ( port );

            addr_ptr  = reinterpret_cast<struct sockaddr*>(&local_sock);
            addr_size = static_cast<int> ( sizeof( local_sock ) );

        } else
        if ( conn_type == conn_type_t::CONN_TYPE_FILE ) {

            os_sock_unlink ( port_str );

            local_file.sun_family = AF_UNIX;
            strncpy ( local_file.sun_path, port_str, sizeof ( local_file.sun_path ) - 1 );

            addr_ptr  = reinterpret_cast<struct sockaddr*>(&local_file);
            addr_size = (int) SUN_LEN( &local_file );

        }

        io_res = bind ( sock, addr_ptr, addr_size );
        if ( io_res == SOCKET_ERROR ) {
            ret_val = false; // Success
        } else {
            ret_val = true; // Success
        }

    }

    #endif

    return ret_val;
}

static bool socket_select ( os_sock_t sock, bool& is_timeout ) {

    bool ret_val = false;

    is_timeout = false;

    fd_set          readfds;
    struct timeval  timeout;

    FD_ZERO ( &readfds );
    FD_SET ( sock, &readfds );

    timeout.tv_sec  = 5;           // once per 20 seconds.
    timeout.tv_usec = 200000;      // 

    int sel_res = select ( (int) (sock + 1), &readfds, NULL, NULL, &timeout );

    if ( sel_res == -1 ) {
        if ( errno == EWOULDBLOCK ) {
            sel_res = 0;
        } else
        if ( errno == EAGAIN ) {
            sel_res = 0;
        }
    }

    if ( sel_res < 0 ) {
        // TTRACE ();
        ret_val = false;
    } else
    if ( sel_res == 0 ) {
        // TTRACE ();
        is_timeout = true;
        ret_val = true;
    } else {
        is_timeout = false;
        ret_val = true;
    }

    return ret_val;
}

static bool socket_accept ( os_sock_t server_sock, os_sock_t& client_sock ) {

    bool       ret_val = false;
    os_sock_t  io_res;

    io_res = accept ( server_sock, NULL, NULL );

    if ( io_res != SOCK_INVALID_SOCK ) {
        client_sock = io_res;
        ret_val = true;
    } 

    return ret_val;
}

static bool socket_connect ( os_sock_t sock, conn_type_t conn_type, const std::string port ) {

    bool ret_val = false;
    int  io_res  = 0;

    struct sockaddr_un addr_file    = {};
    struct sockaddr_in addr_sock    = {};
    struct sockaddr*   con_addr_ptr = nullptr;
    int                con_addr_len = 0;

    if ( conn_type == conn_type_t::CONN_TYPE_FILE ) {
        addr_file.sun_family = AF_UNIX;
        strncpy ( addr_file.sun_path, port.c_str(), UNIX_PATH_MAX-1 );
        con_addr_ptr = (struct sockaddr*) &addr_file;
        con_addr_len = (int) SUN_LEN ( &addr_file );
    }
    if ( conn_type == conn_type_t::CONN_TYPE_SOCK ) {
        int port_ = atoi (port.c_str());
        addr_sock.sin_family = AF_INET;
        addr_sock.sin_port = htons(port_);
        (void)inet_pton ( AF_INET, "127.0.0.1", &addr_sock.sin_addr );
        con_addr_ptr = (struct sockaddr*) &addr_sock;
        con_addr_len = sizeof(addr_sock);
    }

    io_res = connect ( sock, con_addr_ptr, con_addr_len );
    if ( io_res >= 0 ) {
        ret_val = true;
    }
    return ret_val;
}

static bool socket_tx ( os_sock_t sock, const hid::types::storage_t& out_frame, size_t& tx_offset ) {

    bool ret_val = false;

    if ( out_frame.size () > tx_offset ) {
        if ( sock != SOCK_INVALID_SOCK ) {

            const uint8_t* src_pos = static_cast<const uint8_t*> (out_frame.data ());
            src_pos += tx_offset;

            uint32_t data_part;
            data_part = static_cast<uint32_t> (out_frame.size ());
            data_part -= static_cast<uint32_t> (tx_offset);

            int io_res;
            io_res = ::send ( sock, (char*) src_pos, data_part, 0 );

            if ( io_res > 0 ) {
                tx_offset += io_res;
                ret_val = true;
            }

        }
    }

    return ret_val;
}

static bool socket_rx ( os_sock_t sock, hid::types::storage_t& inp_frame, size_t& rx_offset ) {

    bool ret_val = false;

    if ( rx_offset < inp_frame.size () ) {
        if ( sock != SOCK_INVALID_SOCK ) {

            uint8_t* dst_pos = static_cast<uint8_t*> (inp_frame.data ());
            dst_pos += rx_offset;

            uint32_t data_part;
            data_part = static_cast<uint32_t> (inp_frame.size ());
            data_part -= static_cast<uint32_t> (rx_offset);

            int io_res;
            io_res = ::recv ( sock, (char*) dst_pos, data_part, 0 );

            if ( io_res > 0 ) {
                rx_offset += io_res;
                ret_val = true;
            }

        }
    }

    return ret_val;
}

static bool frame_tx ( os_sock_t sock, const hid::types::storage_t& out_frame ) {

    bool ret_val = false;

    if ( sock != SOCK_INVALID_SOCK ) {

        if ( out_frame.size () == 0 ) {
            ret_val = true;
        } else {

            bool    io_res = false;
            size_t  tx_offset = 0;

            for ( ; ; ) {
                io_res = socket_tx ( sock, out_frame, tx_offset );
                if ( ! io_res ) {
                    break;
                }
                if ( tx_offset == out_frame.size () ) {
                    ret_val = true;
                    break;
                }
            }
        }

    }

    return ret_val;
}

static bool frame_rx ( os_sock_t sock, hid::types::storage_t& inp_frame ) {

    bool ret_val = false;

    if ( sock != SOCK_INVALID_SOCK ) {

        if ( inp_frame.size () == 0 ) {
            ret_val = true;
        } else {

            bool    io_res = false;
            size_t  rx_offset = 0;

            for( ; ; ) {
                io_res = socket_rx ( sock, inp_frame, rx_offset );
                if ( ! io_res ) {
                    break;
                }
                if ( rx_offset == inp_frame.size () ) {
                    ret_val = true;
                    break;
                }
            }
        }

    }

    return ret_val;
}

//---------------------------------------------------------------------------//

sock_transaction_t::sock_transaction_t () {

    m_closed = false;
    m_idx    = TRANSACTION_INVALID_IDX;
}

void sock_transaction_t::checkpoint_set ( sock_checkpoint_type_t point_type ) {

    hid::socket::sock_checkpoint_t ref = sock_time_src_t::now();

    switch ( point_type ) {
        case sock_checkpoint_type_t::CHECKPOINT_START:
            m_start_time = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_RX_HDR:
            m_rcv_hdr = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_RX_PAYLOAD:
            m_rcv_payload = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_TX_HDR:
            m_snt_hdr = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_TX_PAYLOAD:
            m_snt_payload = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_COMMIT:
            m_commit_time = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_UNKNOWN:
        default:
            break;
    }
}

bool sock_transaction_t::is_expired () {

    bool ret_val = false;

    sock_checkpoint_t ref = sock_time_src_t::now();

    if ( m_expiration_time < ref ) {
        ret_val = true;
    }

    return ret_val;
}

void sock_transaction_t::close () {

    if ( ! m_closed ) {
        m_closed = true;
        m_commit_time = sock_time_src_t::now();
    }
}

//---------------------------------------------------------------------------//

SocketServer::SocketServer () {
    os_sock_init ();
    m_stop = false;
    m_ev_handler = dummy_ev_handler;
}

SocketServer::~SocketServer () {

    Stop ();
}

bool SocketServer::ConnProcessNew ( os_sock_t server_socket ) {

    bool    ret_val = false;
    bool    is_timeout;
    bool    io_res;

    io_res = socket_select (server_socket, is_timeout);
    if ( ! io_res ) {
        // Unexpected result.
        ret_val = false;
    } else
    if ( is_timeout ) {
        ret_val = true;
    } else {
        os_sock_t client_sock;
        io_res = socket_accept ( server_socket, client_sock );
        if ( ! io_res ) {
            ret_val = false;
        } else {
            sock_thread_t client_handler;
            client_handler = std::async ( std::launch::async, &SocketServer::Shell, this, client_sock );
            m_clients.emplace_back ( std::move ( client_handler ) );
            ret_val = true;
        }
    }

    return ret_val;
}

void SocketServer::Service () {

    os_sock_t server_socket = (os_sock_t) SOCK_INVALID_SOCK;

    bool listen_required = true;
    int  err_cnt_start  = 0;
    int  err_cnt_socket = 0;

    bool io_res;


    while ( ! m_stop ) {

        if ( err_cnt_start > SOCK_MAX_ERRORS_CNT ) {
            break;
        }

        if ( err_cnt_socket > SOCK_MAX_ERRORS_CNT ) {
            break;
        }

        listen_required = false;

        if ( server_socket == SOCK_INVALID_SOCK ) {
            listen_required = true;
            socket_open ( m_conn_type, server_socket );
            io_res = socket_bind ( server_socket, m_conn_type, m_port.c_str() );
            if ( ! io_res ) {
                socket_close( server_socket );
            }

        }

        if ( server_socket == SOCK_INVALID_SOCK ) {
            err_cnt_start++;
            std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
            continue;
        }

        if ( listen_required ) {
            int listen_res;
            listen_res = ::listen ( server_socket, SOMAXCONN );
            if ( listen_res != 0 ) {
                os_sockclose ( server_socket );
                server_socket = SOCK_INVALID_SOCK;
                continue;
            }
        }

        err_cnt_start = 0;

        io_res = ConnProcessNew ( server_socket );
        if ( ! io_res ) {
            err_cnt_socket++;
            os_sockclose ( server_socket );
            server_socket = SOCK_INVALID_SOCK;
            continue;
        }

        err_cnt_socket = 0;

    }

    os_sockclose ( server_socket );

    if ( m_conn_type == conn_type_t::CONN_TYPE_FILE ) {
        os_sock_unlink ( m_port.c_str() );
    }

    m_instance_cnt--;
    return;
}

void SocketServer::SetHandler ( ev_handler_t handler ) {

    m_ev_handler = handler;
}

bool SocketServer::Start ( const char* const port, conn_type_t conn_type ) {

    bool ret_val = false;

    m_controller.lock ();

        try {

            if ( m_instance_cnt == 0 ) {

                m_instance_cnt++;

                m_port = port;
                m_conn_type = conn_type;

                m_server_thread = std::thread ( &SocketServer::Service, this );
                std::this_thread::sleep_for ( std::chrono::milliseconds ( 1 ) );
            }
            
        } catch ( ... ) {
            // TTRACE ("exception");
        }

    m_controller.unlock ();

    return ret_val;
}

void SocketServer::Stop ( void ) {

    m_stop = true;

    if ( m_server_thread.joinable () ) {
        m_server_thread.join ();
    }

    for ( auto &client : m_clients ) {
        if ( client.valid () ) {
            client.get();
        }
    }
    
    m_clients.clear ();
}

bool SocketServer::Shell ( os_sock_t socket ) {

    hid::stream::StreamPrefix     prefix;
    hid::stream::stream_params_t  inp_params = {};
    hid::types::storage_t         inp_hdr;
    hid::types::storage_t         inp_pay;

    hid::stream::stream_params_t  out_params = {};
    hid::types::storage_t         out_hdr;
    hid::types::storage_t         out_pay;

    int  out_err_code;
    bool io_res;

    while ( ! m_stop ) {

        inp_hdr.resize ( prefix.size() );

        io_res = frame_rx ( socket, inp_hdr );
        if ( ! io_res ) {
           break;
        }
        printf ("inp_hdr \r\n");

        io_res = prefix.Valid ( inp_hdr );
        if ( ! io_res ) {
           break;
        }

        prefix.GetParams ( inp_hdr, inp_params );

        inp_pay.resize ( inp_params.len );
        io_res = frame_rx ( socket, inp_pay );
        if ( ! io_res ) {
            break;
        }

        printf ( "inp_pay \r\n" );

        if ( inp_params.command == hid::stream::StreamCmd::STREAM_CMD_PING_REQUEST ) {
            out_params.command = hid::stream::StreamCmd::STREAM_CMD_PING_RESPONSE;
            out_params.code = 0;
            out_pay.resize (0);
        } else 
        if ( inp_params.command == hid::stream::StreamCmd::STREAM_CMD_REQUEST ) {
            (m_ev_handler) ( inp_pay, out_pay, out_err_code );
            out_params.command = hid::stream::StreamCmd::STREAM_CMD_RESPONSE;
            out_params.code = out_err_code;
        } else {
            out_params.command = hid::stream::StreamCmd::STREAM_CMD_ERROR;
            out_params.code = 0;
            out_pay.resize ( 0 );
        }

        out_params.len = static_cast<uint32_t> ( out_pay.size () );

        prefix.SetParams ( out_params, out_hdr );

        io_res = frame_tx ( socket, out_hdr );
        if ( ! io_res ) {
            break;
        }

        printf ( "out_hdr \r\n" );

        io_res = frame_tx ( socket, out_pay );
        if ( ! io_res ) {
            break;
        }

        printf ( "out_pay \r\n" );

        printf ( "! close \r\n" );
    }

    socket_close ( socket );
    return true;

}

bool SocketServer::ConnMoveToExpired () {

    bool acquired;

    acquired = m_controller.try_lock ();

    if( !acquired ) {
        return true;
    }

    try {

        // for( auto pos = m_pending_list.begin (); pos != m_pending_list.end (); ) {
        //     if( pos->is_expired () ) {
        //         m_rejected_list.push_back ( *pos );
        //         pos = m_pending_list.erase ( pos );
        //     } else {
        //         pos++;
        // 
        //     }
        // }

    } catch( ... ) {
    }

    m_controller.unlock ();

    return true;
}

bool SocketServer::ConnProcessExpired () {

    return true;
}

//---------------------------------------------------------------------------//

SocketClient::SocketClient () {

    os_sock_init ();
    m_conn_type = conn_type_t::CONN_TYPE_UNKNOW;
    m_sock = (os_sock_t) SOCK_INVALID_SOCK;
}

SocketClient::~SocketClient () {

    if ( m_sock != SOCK_INVALID_SOCK ) {
        os_sockclose (m_sock);
        m_sock = SOCK_INVALID_SOCK;
    }

}

bool SocketClient::Connect ( const char* const portStr, conn_type_t type ) {

    // Do not connect immediatelly. 
    // Just keep parameters where (and how) to connect.
    // Connection will be established in the transaction.
    m_port = portStr;
    m_conn_type = type;

    return true;
}

void SocketClient::Close () {

    socket_close( m_sock );
}

bool SocketClient::Transaction ( std::chrono::milliseconds delayMs, const hid::types::storage_t& out_fame, hid::types::storage_t& in_frame ) {

    bool ret_val = true;
    bool io_res  = false;

    try {
        hid::types::storage_t         out_hdr;
        hid::types::storage_t         inp_hdr;
        hid::stream::StreamPrefix     prefix;
        hid::stream::stream_params_t  params = {};

        params.command = hid::stream::StreamCmd::STREAM_CMD_REQUEST;
        params.len = static_cast<uint32_t> (out_fame.size ());
        prefix.SetParams ( params, out_hdr );

        io_res = frame_tx ( m_sock, out_hdr );
        if ( ! io_res ) {
            // TTRACE ("Attempt to (re)Connect to server.");
            socket_close ( m_sock );
            socket_open ( m_conn_type, m_sock );
            socket_connect ( m_sock, m_conn_type, m_port );
            io_res = frame_tx ( m_sock, out_hdr );
        }

        if ( ! io_res ) {
            // TTRACE ("Transaction failed. Can't send prefix");
            goto EXIT;
        }

        io_res = frame_tx ( m_sock, out_fame );
        if ( ! io_res ) {
            // TTRACE ("Transaction failed. Can't send payload");
            goto EXIT;
        }

        inp_hdr.resize ( prefix.size() );
        io_res = frame_rx ( m_sock, inp_hdr );
        if ( ! io_res ) {
            // TTRACE ("Transaction failed. Can't read prefix");
            goto EXIT;
        }

        io_res = prefix.Valid ( inp_hdr );
        if ( ! io_res ) {
            // TTRACE ("Transaction failed. Wrong prefix received.");
            goto EXIT;
        }

        prefix.GetParams ( inp_hdr, params );
        in_frame.resize ( params.len );

        io_res = frame_rx ( m_sock, in_frame );
        if ( ! io_res ) {
            // TTRACE ("Transaction failed. Can't receive payload.");
            goto EXIT;
        }

        ret_val = true;

    }   catch( ... ) {
        // TTRACE ("Transaction failed. Internal error.");
    }

EXIT:
    if ( ! ret_val ) {
        // TTRACE ("Transaction failed. Close socked.");
        socket_close ( m_sock );
    }
    return ret_val;
}

}
}
