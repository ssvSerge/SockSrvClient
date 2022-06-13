#include <SockServerClient.h>
#include <StreamPrefix.h>


#define  SUN_LEN(p)  ((size_t) (( (struct sockaddr_un*) NULL)->sun_path) + strlen ((p)->sun_path))


namespace hid {
namespace socket {

//---------------------------------------------------------------------------//

constexpr size_t TRANSACTION_INVALID_IDX = 0;
constexpr int    SOCK_MAX_ERRORS_CNT = 15;

//---------------------------------------------------------------------------//

sock_transaction_t::sock_transaction_t () {

    m_closed = false;
    m_sock   = SOCK_INVALID_SOCK;
    m_idx    = TRANSACTION_INVALID_IDX;
}

sock_transaction_t::~sock_transaction_t () {

}

sock_transaction_t::sock_transaction_t ( const sock_transaction_t& ref ) {
    move_to_me ( ref );
};

void sock_transaction_t::move_to_me ( const sock_transaction_t& ref ) {

    this->m_start_time      = ref.m_start_time;
    this->m_commit_time     = ref.m_commit_time;
    this->m_expiration_time = ref.m_expiration_time;

    this->m_idx  = ref.m_idx;
    this->m_sock = ref.m_sock;

    this->inp_data = std::move ( const_cast<sock_transaction_t&> (ref).inp_data );
    this->out_data = std::move ( const_cast<sock_transaction_t&> (ref).out_data );

    const_cast<sock_transaction_t&> (ref).m_idx  = TRANSACTION_INVALID_IDX;
    const_cast<sock_transaction_t&> (ref).m_sock = SOCK_INVALID_SOCK;
}

void sock_transaction_t::checkpoint_set ( sock_checkpoint_type_t point_type ) {

    ::hid::socket::sock_checkpoint_t ref = sock_time_src_t::now();

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

static bool socket_open ( conn_type_t conn_type, const char* const port_str, bool bind_requird, os_sock_t& sock ) {

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
    } else {
        if ( ! bind_requird ) {
            ret_val = true;
        } else {
            if ( socket_bind ( sock, conn_type, port_str ) ) {
                ret_val = true;
            }
        }
    }


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
        ret_val = false;
    } else
    if ( sel_res == 0 ) {
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

    if ( client_sock != (os_sock_t) (-1) ) {
        client_sock = io_res;
        ret_val = true;
    } 

    return ret_val;
}

//---------------------------------------------------------------------------//

SocketServer::SocketServer () {

    os_sock_init ();
    m_stop = false;
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

bool SocketServer::check_server_socket ( os_sock_t& server_socket, bool& restarted ) {

    bool ret_val = true;

    restarted = false;
    if ( server_socket == SOCK_INVALID_SOCK ) {
       socket_open ( m_conn_type, m_port.c_str (), true, server_socket );
       if ( server_socket == SOCK_INVALID_SOCK ) {
           std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
       } else {
       }
    }

    return ret_val;
}

void SocketServer::Service () {

    os_sock_t server_socket = (os_sock_t) SOCK_INVALID_SOCK;
    bool io_res;

    while ( ! m_stop ) {

        io_res = check_server_socket (server_socket );

        if ( server_socket == SOCK_INVALID_SOCK ) {

            socket_open ( m_conn_type, m_port.c_str (), true, server_socket );

            if ( server_socket == SOCK_INVALID_SOCK ) {

                err_cnt++;
                std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
                if ( err_cnt > SOCK_MAX_ERRORS_CNT ) {
                    break;
                }

                continue;
            }

        }

    }

    os_sockclose ( server_socket );
    m_instance_cnt--;
    return;

    // int         err_cnt = 0;
    // bool        server_started = false;
    // os_sock_t   server_socket = (os_sock_t) SOCK_INVALID_SOCK;
    // 
    // while ( err_cnt < SOCK_MAX_ERRORS_CNT ) {
    //     server_started = socket_open ( m_conn_type, m_port.c_str (), true, server_socket );
    //     if ( server_started ) {
    //         break;
    //     }
    //     err_cnt++;
    //     std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
    // }
    // 
    // int  io_res  = 0;
    // io_res = ::listen ( server_socket, SOMAXCONN );
    // 
    // if ( io_res != SOCKET_ERROR ) {
    //     if ( server_started ) {
    //         while( !m_stop ) {
    //             ConnProcessNew ( server_socket );
    //         }
    //     }
    // }
    // 
    // os_sockclose ( server_socket );
    // m_instance_cnt--;
    // return;
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

    std::this_thread::sleep_for( std::chrono::milliseconds(15000) );

    os_sockclose ( socket );
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

    bool                acquired;
    sock_transaction_t  tr;
    // msg_header_t        hdr;

    while ( true ) {

        if ( ! m_controller.try_lock () ) {
            break;
        }

        acquired = false;

        try {
            // if( m_rejected_list.size () > 0 ) {
            //     tr.move_to_me ( m_rejected_list.front () );
            //     m_rejected_list.pop_front ();
            //     acquired = true;
            // }
        } catch( ... ) {
        }

        m_controller.unlock ();

        if ( !acquired ) {
            break;
        }

        tr.out_data.clear ();

        // hdr.format ( frame_type_t::FRAME_TYPE_TIMEOUT, 0, 0, 0 );

        // SendFrame ( tr.m_sock, hdr.m_hdr );

        os_sockclose ( tr.m_sock );
    }

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

    m_port = portStr;
    m_conn_type = type;

    return true;
}

bool SocketClient::Transaction ( std::chrono::milliseconds delayMs, const ::hid::types::storage_t& out_fame, ::hid::types::storage_t& in_frame ) {

    ::hid::types::storage_t out_hdr;
    ::hid::types::storage_t inp_hdr;

    {   // Prepare Frame Prefix.
        ::hid::stream::StreamPrefix     prefix;
        ::hid::stream::stream_params_t  out_params;

        out_params.command = ::hid::stream::StreamCmd::STREAM_CMD_REQUEST;
        out_params.code = 0;
        out_params.len = static_cast<uint32_t> (out_fame.size());
        prefix.Format ( out_params, out_hdr );
    }

    send_frame ( out_hdr, true );
    send_frame ( out_fame, false );
    recv_frame ( inp_hdr );
    recv_frame ( in_frame );

    return true;
}

bool SocketClient::send_frame ( const ::hid::types::storage_t& out_frame, bool re_connect_enabled ) {

    bool io_res = true;

    if ( out_frame.size () > 0 ) {

        bool    io_res     = false;
        size_t  tx_offset  = 0;
        bool    first_send = true;

        for ( ; ; ) {

            io_res = sock_tx ( out_frame, tx_offset );

            if ( first_send ) {
                first_send = false;
                if ( ! io_res ) {
                   sock_close ();
                   sock_open ();
                   io_res = sock_connect ();
               }
            }

            if ( ! io_res ) {
                break;
            }

            if ( tx_offset == out_frame.size () ) {
                break;
            }

        }

    }

    return io_res;
}

bool SocketClient::recv_frame ( ::hid::types::storage_t& inp_frame ) {

    bool io_res = true;

    if ( inp_frame.size () > 0 ) {

        bool    io_res     = false;
        size_t  rx_offset  = 0;

        for ( ; ; ) {

            io_res = sock_rx ( inp_frame, rx_offset );

            if ( ! io_res ) {
                break;
            }
            if ( rx_offset == inp_frame.size () ) {
                break;
            }

        }

    }

    return io_res;
}

bool SocketClient::sock_tx ( const ::hid::types::storage_t& out_frame, size_t& tx_offset ) {

    bool ret_val = false;

    if ( out_frame.size() > tx_offset ) {
        if ( m_sock != SOCK_INVALID_SOCK ) {

            const uint8_t* src_pos = static_cast<const uint8_t*> (out_frame.data ());
            src_pos += tx_offset;

            uint32_t data_part;
            data_part  = static_cast<uint32_t> (out_frame.size());
            data_part -= static_cast<uint32_t> (tx_offset);

            int io_res;
            io_res = ::send ( m_sock, (char*)src_pos, data_part, 0 );

            if ( io_res > 0 ) {
                tx_offset += io_res;
                ret_val = true;
            }

        }
    }

    return ret_val;
}

bool SocketClient::sock_rx ( ::hid::types::storage_t& inp_frame, size_t& rx_offset ) {

    bool ret_val = false;

    if ( rx_offset < inp_frame.size () ) {
        if ( m_sock != SOCK_INVALID_SOCK ) {

            uint8_t* dst_pos = static_cast<uint8_t*> (inp_frame.data ());
            dst_pos += rx_offset;

            uint32_t data_part;
            data_part  = static_cast<uint32_t> (inp_frame.size());
            data_part -= static_cast<uint32_t> (rx_offset);

            int io_res;
            io_res = ::recv ( m_sock, (char*)dst_pos, data_part, 0 );

            if ( io_res > 0 ) {
                rx_offset += io_res;
                ret_val = true;
            }

        }
    }
    return ret_val;
}

void SocketClient::sock_close () {

    if ( m_sock != SOCK_INVALID_SOCK ) {
        os_sockclose ( m_sock );
        m_sock = SOCK_INVALID_SOCK;
    }

}

void SocketClient::sock_open () {

    bool io_res;
    io_res = socket_open( m_conn_type, m_port.c_str(), false, m_sock );

    if ( ! io_res ) {
        m_sock = SOCK_INVALID_SOCK;
    }
}

bool SocketClient::sock_connect () {

    bool ret_val = false;
    int  io_res  = 0;

    struct sockaddr_un serveraddr = {};
    serveraddr.sun_family = AF_UNIX;
    strncpy ( serveraddr.sun_path, m_port.c_str(), UNIX_PATH_MAX-1 );

    io_res = connect ( m_sock, (struct sockaddr*)&serveraddr, (int)SUN_LEN(&serveraddr) );
    if ( io_res >= 0 ) {
        ret_val = true;
    }

    return ret_val;
}


}
}
