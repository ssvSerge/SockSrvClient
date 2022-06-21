#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <SockServerClient.h>
#include <StreamPrefix.h>

#define  TTRACE              printf

using namespace std::chrono_literals;


namespace hid {
namespace socket {

//---------------------------------------------------------------------------//

constexpr int    SOCK_MAX_ERRORS_CNT    = 15;

//---------------------------------------------------------------------------//

uint32_t         SOCK_TEXT_TX_DELAY     = 0;

//---------------------------------------------------------------------------//

static void dummy_ev_handler ( 
    IN    MANDATORY const hid::types::storage_t& in_data, 
    OUT   MANDATORY hid::types::storage_t& out_data, 
    OUT   MANDATORY uint32_t&           error_code 
) {
    UNUSED ( in_data );
    out_data.resize(256);
    error_code = 100;
}

//---------------------------------------------------------------------------//

static bool get_timeval ( 
    IN    MANDATORY checkpoint_t        end_time,
    OUT   MANDATORY struct timeval&     tv 
) {

    bool ret_val = false;

    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    checkpoint_t curr_time = hid::socket::time_source_t::now ();

    if ( curr_time < end_time ) {

        std::chrono::duration time_diff = (end_time - curr_time);
        std::chrono::microseconds mks = std::chrono::duration_cast<std::chrono::microseconds>(time_diff);

        if ( mks.count () > 1000 ) {
            tv.tv_sec  = static_cast<long> ( mks.count() / 1000000);
            tv.tv_usec = static_cast<long> ( mks.count() % 1000000);
            ret_val = true;
        }

    }

    return ret_val;
}

static std::string tp_to_string ( 
    IN    OPTIONAL const char* const    prefix, 
    IN    MANDATORY const checkpoint_t& time
) {

    std::time_t  tt   = hid::socket::time_source_t::to_time_t (time);
    std::tm      tm   = *std::gmtime(&tt);
    std::stringstream ss;

    if ( prefix != nullptr ) {
        ss << prefix;
    }

    ss << std::put_time( &tm, "UTC: %Y-%m-%d %H:%M:%S \n" );
    return ss.str();
}

static std::string dur_to_string ( 
    IN    OPTIONAL const char* const    prefix, 
    IN    MANDATORY const checkpoint_t& start,
    IN    MANDATORY const checkpoint_t& end
) {

    std::string ret_val;

    uint64_t seconds_cnt = std::chrono::duration_cast<std::chrono::seconds>(end.time_since_epoch()).count();
    if ( seconds_cnt > 0 ) {
        if ( end > start ) {
            auto diff = (end - start);
            auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);

            if ( prefix != nullptr ) {
                ret_val = prefix;
            }

            ret_val += "(+";
            ret_val += std::to_string ( diff_ms.count() );
            ret_val += "ms) \n";

        }
    }

    return ret_val;
}

//---------------------------------------------------------------------------//

static void socket_open ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY conn_type_t         conn_type, 
    INOUT MANDATORY os_sock_t&          sock 
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        int  af_mode = AF_UNSPEC;
        if ( conn_type == conn_type_t::CONN_TYPE_SOCK ) {
            af_mode = AF_INET;
        } else
        if ( conn_type == conn_type_t::CONN_TYPE_FILE ) {
            af_mode = AF_UNIX;
        }

        sock_state = sock_state_t::SOCK_ERR_OPEN;

        sock = ::socket ( af_mode, SOCK_STREAM, 0 );
        if ( sock_valid (sock) ) {
            os_sock_nodelay ( sock );
            sock_state = sock_state_t::SOCK_OK;
        }

    }
}

static void socket_nodelay ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    INOUT MANDATORY os_sock_t&          sock 
) {
    if ( sock_state == sock_state_t::SOCK_OK ) {
        os_sockclose (sock);
    }
}

static void socket_bind ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY conn_type_t         conn_type, 
    IN    MANDATORY const char* const   port_str 
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        sockaddr_in         local_sock  = {};
        struct sockaddr_un  local_file  = {};

        struct sockaddr*    addr_ptr    = nullptr;
        int                 addr_size   = 0;

        if ( conn_type == conn_type_t::CONN_TYPE_SOCK ) {

            u_short port = static_cast<u_short> ( atoi(port_str) );

            local_sock.sin_family = AF_INET;
            local_sock.sin_port   = ( htons (port) );

            addr_ptr  = reinterpret_cast<struct sockaddr*>( &local_sock );
            addr_size = static_cast<int> ( sizeof(local_sock) );

        } else
        if ( conn_type == conn_type_t::CONN_TYPE_FILE ) {

            sock_unlink ( port_str );

            local_file.sun_family = AF_UNIX;
            strncpy ( local_file.sun_path, port_str, sizeof ( local_file.sun_path ) - 1 );

            addr_ptr  = reinterpret_cast<struct sockaddr*>(&local_file);
            addr_size = (int) SUN_LEN( &local_file );

        }

        sock_state = sock_state_t::SOCK_ERR_BIND;

        int io_res = ::bind ( sock, addr_ptr, addr_size );
        if ( io_res != -1 ) {
            sock_state = sock_state_t::SOCK_OK;
        }
    }
    
}

static void socket_listen ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock 
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        sock_state = sock_state_t::SOCK_ERR_LISTEN;

        int listen_res = ::listen ( sock, SOMAXCONN );
        if ( listen_res == 0 ) {
            sock_state = sock_state_t::SOCK_OK;
        }

    }
}

static void socket_wait ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY duration_ms_t       timeout_ns
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        struct timeval  timeout;
        fd_set          readfds;

        FD_ZERO ( &readfds );
        FD_SET ( sock, &readfds );

        std::chrono::microseconds mks = std::chrono::duration_cast<std::chrono::microseconds>(timeout_ns);

        timeout.tv_sec = static_cast<long> (mks.count () / 1000000);
        timeout.tv_usec = static_cast<long> (mks.count () % 1000000);

        int select_res = select ( static_cast<int>(sock + 1), &readfds, NULL, NULL, &timeout );

        sock_state = sock_state_t::SOCK_ERR_SELECT;

        if ( select_res > 0 ) {
            sock_state = sock_state_t::SOCK_OK;
        } else
        if ( select_res == 0 ) {
            sock_state = sock_state_t::SOCK_ERR_TIMEOUT;
        } else {
            int select_err = sock_error ();
            if ( (select_err == EINTR) || (select_err == EWOULDBLOCK) ) {
                sock_state = sock_state_t::SOCK_ERR_TIMEOUT;
            } else {
                sock_state = sock_state_t::SOCK_ERR_SELECT;
            }
        }
    }
}

static void socket_accept ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           server_sock, 
    OUT   MANDATORY os_sock_t&          client_sock
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        sock_state = sock_state_t::SOCK_ERR_ACCEPT;

        client_sock = accept ( server_sock, NULL, NULL );

        if ( sock_valid(client_sock) ) {
            sock_state = sock_state_t::SOCK_OK;
        }
    }
}

static void socket_connect ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY conn_type_t         conn_type, 
    IN    MANDATORY const std::string   port 
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        struct sockaddr_un addr_file    = {};
        struct sockaddr_in addr_sock    = {};
        struct sockaddr*   con_addr_ptr = nullptr;
        int                con_addr_len = 0;

        if ( conn_type == conn_type_t::CONN_TYPE_FILE ) {
            addr_file.sun_family = AF_UNIX;
            strncpy ( addr_file.sun_path, port.c_str(), sizeof(addr_file.sun_path) - 1);
            con_addr_ptr = reinterpret_cast<struct sockaddr*> (&addr_file);
            con_addr_len = (int) SUN_LEN ( &addr_file );
        } else
        if ( conn_type == conn_type_t::CONN_TYPE_SOCK ) {
            uint16_t port_ = static_cast<uint16_t> ( atoi (port.c_str()) );
            addr_sock.sin_family = AF_INET;
            addr_sock.sin_port = htons(port_);
            (void)inet_pton ( AF_INET, "127.0.0.1", &addr_sock.sin_addr );
            con_addr_ptr = reinterpret_cast<struct sockaddr*> (&addr_sock);
            con_addr_len = sizeof(addr_sock);
        }

        sock_state = sock_state_t::SOCK_ERR_CONNECT;

        if ( con_addr_ptr != nullptr ) {
            int io_res = ::connect ( sock, con_addr_ptr, con_addr_len );
            if ( io_res == 0 ) {
                sock_state = sock_state_t::SOCK_OK;
            }
        }

    }
}

static void frame_wait ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        struct timeval tv;
        fd_set readfds;

        FD_ZERO ( &readfds );
        FD_SET ( sock, &readfds );

        tv.tv_sec  = 1;
        tv.tv_usec = 0;

        int io_res = select ( static_cast<int>(sock + 1), &readfds, NULL, NULL, &tv );

        if ( io_res < 0 ) {
            TTRACE ("Socket error: SELECT failed. \n");
            sock_state = sock_state_t::SOCK_ERR_SELECT;
        } else
        if ( io_res == 0 ) {
            sock_state = sock_state_t::SOCK_ERR_TIMEOUT;
        }
    }
}

static void frame_tx ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY const hid::types::storage_t& out_frame 
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        if ( out_frame.size () > 0 ) {

            const uint8_t* tx_pos = nullptr;
            int     tx_part = 0;
            size_t  tx_cnt  = 0;

            sock_blocking (sock);

            tx_cnt = 0;

            for ( ; ; ) {

                if ( tx_cnt == out_frame.size () ) {
                    break;
                }

                tx_pos  = static_cast<const uint8_t*> (out_frame.data ()+tx_cnt);

                {   // Testing of DELAYS in the data transfer. 
                    if ( SOCK_TEXT_TX_DELAY != 0 ) {
                        tx_part = 1;
                        std::this_thread::sleep_for ( std::chrono::milliseconds ( SOCK_TEXT_TX_DELAY ) );
                    } else {
                        tx_part = static_cast<int> (out_frame.size () - tx_cnt);
                    }
                }


                // std::this_thread::sleep_for( std::chrono::milliseconds(1) );
                // tx_part = 1;

                int io_res = ::send ( sock, reinterpret_cast<const char*>(tx_pos), tx_part, 0 );

                if ( io_res < 0 ) {
                    sock_state = sock_state_t::SOCK_ERR_TX;
                    break;
                }

                if ( io_res == 0 ) {
                    sock_state = sock_state_t::SOCK_ERR_CLOSED;
                    break;
                }

                tx_cnt += io_res;
            }

        }

    }
}

static void frame_rx ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY duration_ms_t       delay,
    OUT   MANDATORY hid::types::storage_t& inp_frame 
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        if ( inp_frame.size () > 0 ) {

            checkpoint_t end_time = hid::socket::time_source_t::now() + delay;

            char*   rx_pos    = nullptr;
            int     rx_part   = 0;
            size_t  rx_cnt    = 0;

            sock_nonblocking (sock);

            rx_cnt = 0;
            for ( ; ; ) {

                if ( rx_cnt == inp_frame.size () ) {
                    break;
                }

                rx_pos  = reinterpret_cast<char*> ( inp_frame.data () + rx_cnt );
                rx_part = static_cast<int> (inp_frame.size () - rx_cnt);

                int io_res = ::recv ( sock, rx_pos, rx_part, 0);

                if ( io_res > 0 ) {
                    rx_cnt += io_res;
                    continue;
                } 

                if ( io_res == 0 ) {
                    sock_state = sock_state_t::SOCK_ERR_CLOSED;
                    break;
                }

                io_res = sock_error();
                if ( io_res == EAGAIN ) {
                    continue;
                }

                if ( io_res != EWOULDBLOCK ) {
                    sock_state = sock_state_t::SOCK_ERR_RX;
                    break;
                }

                struct timeval tv;
                fd_set readfds;

                if ( ! get_timeval ( end_time, tv ) ) {
                    sock_state = sock_state_t::SOCK_ERR_TIMEOUT;
                    break;
                }

                FD_ZERO(&readfds);
                FD_SET(sock, &readfds);

                io_res = select(static_cast<int>(sock+1), &readfds, NULL, NULL, &tv);

                if ( io_res < 0 ) {
                    break;
                } else
                if ( io_res == 0 ) {
                    // break;
                } 

                // data arrived.
            }

            sock_blocking (sock);
        }

    }
}

static void frame_rx ( 
    INOUT MANDATORY sock_state_t&       sock_state, 
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY checkpoint_t        exptiration_time,
    OUT   MANDATORY hid::types::storage_t& inp_frame 
) {

    if ( sock_state == sock_state_t::SOCK_OK ) {

        checkpoint_t curr_time = hid::socket::time_source_t::now ();

        if ( curr_time >= exptiration_time ) {
            sock_state = sock_state_t::SOCK_ERR_TIMEOUT;
        } else {
            duration_ms_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(exptiration_time - curr_time);
            frame_rx ( sock_state, sock, ms, inp_frame );
        }
    }
}

//---------------------------------------------------------------------------//

sock_transaction_t::sock_transaction_t () {
    inp_cmd = 0;
}

void sock_transaction_t::start ( duration_ms_t expiration_default_ms ) {

    tv_start = hid::socket::time_source_t::now ();
    tv_expiration = tv_start + expiration_default_ms;
}

void sock_transaction_t::checkpoint_set ( sock_checkpoint_type_t point_type ) {

    checkpoint_t ref = hid::socket::time_source_t::now();

    switch ( point_type ) {
        case sock_checkpoint_type_t::CHECKPOINT_START:
            tv_start = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_RX_HDR:
            tv_rcv_hdr = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_RX_PAYLOAD:
            tv_rcv_pay = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_EXEC:
            tv_exec = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_TX_HDR:
            tv_snt_hdr = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_TX_PAYLOAD:
            tv_snt_pay = ref;
            break;
        case sock_checkpoint_type_t::CHECKPOINT_UNKNOWN:
        default:
            break;
    }
}

void sock_transaction_t::expiration_set ( duration_ms_t expiration_ms ) {

    hid::socket::checkpoint_t   expiration_tp;
    expiration_tp = tv_start + expiration_ms;
    tv_expiration = expiration_tp;
}

void sock_transaction_t::reset ( void ) {

    checkpoint_t zero_val = {};

    inp_hdr.clear ();
    inp_pay.clear ();
    out_hdr.clear ();
    out_pay.clear ();

    inp_cmd       = 0;

    tv_start      = zero_val;
    tv_rcv_hdr    = zero_val;
    tv_rcv_pay    = zero_val;
    tv_exec       = zero_val;
    tv_snt_hdr    = zero_val;
    tv_snt_pay    = zero_val;
    tv_expiration = zero_val;
}

//---------------------------------------------------------------------------//

SocketServer::SocketServer () {
    sock_init ();
    m_stop       = false;
    m_ev_handler = dummy_ev_handler;
    m_conn_type  = conn_type_t::CONN_TYPE_UNKNOW;
    m_instance_active = false;
}

SocketServer::~SocketServer () {

    Stop ();
}

void SocketServer::StartClient ( sock_state_t& sock_state, os_sock_t client_sock ) {

    if ( sock_state == sock_state_t::SOCK_OK ) {
        sock_thread_t client_handler;
        client_handler = std::async ( std::launch::async, &SocketServer::Shell, this, client_sock );
        m_clients.emplace_back ( std::move ( client_handler ) );
    }
}

void SocketServer::Service () {

    os_sock_t server_socket = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
    bool to_log = true;

    int err_cnt_start = 0;

    for ( ; ; ) {

        if ( m_stop ) {
            TTRACE ( "Server: Stop requested. \n" );
            break;
        }

        if ( err_cnt_start > SOCK_MAX_ERRORS_CNT ) {
            TTRACE ( "Server: Too many errors. \n" );
            break;
        }

        if ( ! sock_valid(server_socket) ) {

            sock_state_t init_res = sock_state_t::SOCK_OK;
            socket_open   ( init_res, m_conn_type, server_socket );
            socket_bind   ( init_res, server_socket, m_conn_type, m_port.c_str() );
            socket_listen ( init_res, server_socket );

            if ( init_res != sock_state_t::SOCK_OK ) {
                TTRACE ( "Server: Failed to open server socket. \n" );
                err_cnt_start++;
                os_sockclose (server_socket);
                std::this_thread::sleep_for ( 1s );
                continue;
            }

        }

        err_cnt_start = 0;

        {   // Accept new connection(s)
            os_sock_t client_sock = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
            sock_state_t conn_res = sock_state_t::SOCK_OK;
            m_instance_active = true;

            if ( to_log ) {
                to_log = false;
                TTRACE ( "Server: Ready to accept new connections. \n" );
            }

            socket_wait   ( conn_res, server_socket, std::chrono::seconds(1) );
            socket_accept ( conn_res, server_socket, client_sock );
            StartClient   ( conn_res, client_sock );

            if ( conn_res == sock_state_t::SOCK_OK ) {
                continue;
            } else
            if ( conn_res != sock_state_t::SOCK_ERR_TIMEOUT ) {
                TTRACE ( "Server: Failed to handle new connections. \n" );
                break;
            }

        }

    }

    os_sockclose ( server_socket );

    if ( m_conn_type == conn_type_t::CONN_TYPE_FILE ) {
        sock_unlink ( m_port.c_str() );
    }

    TTRACE ( "Server: Down. \n" );

    m_instance_active = false;
}

void SocketServer::SetHandler ( ev_handler_t handler ) {

    m_ev_handler = handler;
}

bool SocketServer::Start ( const char* const port, conn_type_t conn_type ) {

    bool ret_val = false;

    m_controller.lock ();

        try {

            if ( ! m_instance_active ) {

                m_stop = false;

                m_port = port;
                m_conn_type = conn_type;

                m_server_thread = std::thread ( &SocketServer::Service, this );

                while ( ! m_instance_active ) {
                    std::this_thread::sleep_for ( std::chrono::milliseconds (100) );
                }
            }
            
        } catch ( ... ) {
            TTRACE ("Server: Exception in Server Start. \n");
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

void SocketServer::ShellCmdStart ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr ) {

    UNUSED (socket);
    UNUSED (state);

    try {
        tr.start ( SOCK_COMM_TIMEOUT );
        tr.inp_hdr.resize ( hid::stream::Prefix::PrefixSize() );
    } catch ( ... ) {
        TTRACE ( "Server: Exception in Cmd Start. \n" );
        state = sock_state_t::SOCK_ERR_GENERAL;
    }
}

void SocketServer::ShellReadPrefix ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr ) {
    if ( state == sock_state_t::SOCK_OK ) {
        try {

            frame_rx ( state, socket, tr.tv_expiration, tr.inp_hdr );
            if ( state == sock_state_t::SOCK_OK ) {
                if ( hid::stream::Prefix::Valid(tr.inp_hdr) ) {

                    bool expiration_set;
                    hid::stream::params_t params;

                    hid::stream::Prefix::ExpirationTimeValid ( tr.inp_hdr, expiration_set );
                    if ( expiration_set ) {
                        hid::stream::duration_ms_t  duration_ms;
                        hid::stream::Prefix::ExpirationTimeGet ( tr.inp_hdr, duration_ms );
                        tr.expiration_set( duration_ms );
                    }

                    hid::stream::Prefix::GetParams ( tr.inp_hdr, params );
                    tr.inp_pay.resize ( params.len );
                    tr.inp_cmd = static_cast<int> (params.command);

                    tr.checkpoint_set ( sock_checkpoint_type_t::CHECKPOINT_RX_HDR );
                } else {
                    TTRACE ( "Server: Wrong prefix received. \n" );
                    state = sock_state_t::SOCK_ERR_SYNC;
                }
            } else {
                TTRACE ("Server: Failed to Read Prefix. \n");
            }

        } catch ( ... ) {
            TTRACE ( "Server: Exception in Read Prefix. \n" );
            state = sock_state_t::SOCK_ERR_GENERAL;
        }
    }
}

void SocketServer::ShellReadPayload ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr ) {
    if ( state == sock_state_t::SOCK_OK ) {
        try {
            frame_rx ( state, socket, tr.tv_expiration, tr.inp_pay );
            tr.checkpoint_set ( sock_checkpoint_type_t::CHECKPOINT_RX_PAYLOAD );
        } catch ( ... ) {
            TTRACE ( "Server: Exception in Read Payload. \n" );
            state = sock_state_t::SOCK_ERR_GENERAL;
        }
    }
}

void SocketServer::ShellCmdExec ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr ) {

    UNUSED (socket);

    if ( state == sock_state_t::SOCK_OK ) {
        
        try {

            hid::stream::cmd_t out_cmd = hid::stream::cmd_t::STREAM_CMD_ERROR;
            uint32_t out_code = static_cast<uint32_t> (hid::stream::cmd_t::STREAM_CMD_ERROR);

            {   // Exec command
                hid::stream::cmd_t inp_cmd = static_cast<hid::stream::cmd_t> (tr.inp_cmd);

                tr.out_pay.clear();

                if ( inp_cmd == hid::stream::cmd_t::STREAM_CMD_PING_REQUEST ) {
                    out_cmd  = hid::stream::cmd_t::STREAM_CMD_PING_RESPONSE;
                    out_code = 0;
                } else 
                if ( inp_cmd == hid::stream::cmd_t::STREAM_CMD_REQUEST ) {
                    out_cmd  = hid::stream::cmd_t::STREAM_CMD_RESPONSE;
                    (m_ev_handler) ( tr.inp_pay, tr.out_pay, out_code );
                }
            }

            tr.checkpoint_set ( sock_checkpoint_type_t::CHECKPOINT_EXEC );

            {   // Format response.
                hid::stream::params_t params;
                params.command = out_cmd;
                params.code    = out_code;
                params.len     = static_cast<uint32_t> (tr.out_pay.size() );

                hid::stream::Prefix::SetParams ( params, tr.out_hdr );

            }

        } catch( ... ) {
            TTRACE ( "Server: Exception in Exec command. \n" );
            state = sock_state_t::SOCK_ERR_EXEC;
        }


    }
}

void SocketServer::ShellSendPrefix ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr ) {
    if ( state == sock_state_t::SOCK_OK ) {
        frame_tx ( state, socket, tr.out_hdr );
        tr.checkpoint_set ( sock_checkpoint_type_t::CHECKPOINT_TX_HDR );
        if ( state != sock_state_t::SOCK_OK ) {
            TTRACE ( "Server: Failed to send prefix. \n" );
        }
    }
}

void SocketServer::ShellSendPayload ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr ) {
    if ( state == sock_state_t::SOCK_OK ) {
        frame_tx ( state, socket, tr.out_pay );
        tr.checkpoint_set ( sock_checkpoint_type_t::CHECKPOINT_TX_PAYLOAD );
        if ( state != sock_state_t::SOCK_OK ) {
            TTRACE ( "Server: Failed to send payload. \n" );
        }
    }
}

void SocketServer::LogTransaction ( const sock_transaction_t& tr, const sock_state_t conn_state ) {

    std::string log_msg;

    log_msg += tp_to_string  ( "Shell start:   ", tr.tv_start );
    log_msg += dur_to_string ( "Shell rcv HDR: ", tr.tv_start, tr.tv_rcv_hdr );
    log_msg += dur_to_string ( "Shell rcv PAY: ", tr.tv_start, tr.tv_rcv_pay );
    log_msg += dur_to_string ( "Shell exec:    ", tr.tv_start, tr.tv_exec );
    log_msg += dur_to_string ( "Shell snt HDR: ", tr.tv_start, tr.tv_snt_hdr );
    log_msg += dur_to_string ( "Shell snt PAY: ", tr.tv_start, tr.tv_snt_pay );
    log_msg += dur_to_string ( "Shell Exp:     ", tr.tv_start, tr.tv_expiration );
    log_msg += "Shell code =   ";
    log_msg += std::to_string ( static_cast<uint32_t>(conn_state) );
    log_msg += "\n";
    log_msg += "\n";

    #if 0
        std::cout << log_msg;
    #else
        printf ("Server: Transaction done. \n");
        fflush (stdout);
    #endif
}

void SocketServer::ShellClose ( os_sock_t socket, const sock_state_t& state ) {

    if ( sock_valid (socket) ) {

        if ( state != sock_state_t::SOCK_ERR_CLOSED ) {

            hid::stream::params_t   params;
            hid::types::storage_t   out_frame;

            params.command = hid::stream::cmd_t::STREAM_CMD_ERROR;
            params.code    = static_cast<uint32_t> (state);
            params.len     = 0;

            hid::stream::Prefix::SetParams ( params, out_frame );

            sock_state_t resp = sock_state_t::SOCK_OK;
        
            frame_tx ( resp, socket, out_frame );
            if ( resp == sock_state_t::SOCK_OK ) {
                TTRACE ( "Server: Sent CMD_ERROR. \n" );
            }
        }

    }
}

bool SocketServer::Shell ( os_sock_t socket ) {

    sock_transaction_t tr;

    TTRACE ("Server: New client connected. \n");

    for ( ; ; ) {

        if ( m_stop ) {
            TTRACE ("Server: Command STOP received. \n");
            break;
        }

        sock_state_t conn_state = sock_state_t::SOCK_OK;

        frame_wait ( conn_state, socket );
        if ( conn_state == sock_state_t::SOCK_ERR_TIMEOUT ) {
            continue;
        }
        if ( conn_state != sock_state_t::SOCK_OK ) {
            TTRACE ( "Server: Shell failed. \n" );
            break;
        }

        ShellCmdStart    ( socket, conn_state, tr );
        ShellReadPrefix  ( socket, conn_state, tr );
        ShellReadPayload ( socket, conn_state, tr );
        ShellCmdExec     ( socket, conn_state, tr );
        ShellSendPrefix  ( socket, conn_state, tr );
        ShellSendPayload ( socket, conn_state, tr );
        LogTransaction   ( tr, conn_state );

        tr.reset();

        if ( conn_state != sock_state_t::SOCK_OK ) {
            ShellClose (socket, conn_state);
            break;
        }

    }

    os_sockclose ( socket );
    TTRACE ("Server: Client shell closed. \n");
    return true;

}

//---------------------------------------------------------------------------//

SocketClient::SocketClient () {

    sock_init ();
    m_conn_type = conn_type_t::CONN_TYPE_UNKNOW;
    m_sock = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
}

SocketClient::~SocketClient () {

    if ( sock_valid (m_sock) ) {
        os_sockclose (m_sock);
        m_sock = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
    }

}

bool SocketClient::Connect ( const char* const portStr, conn_type_t type ) {

    sock_state_t state = sock_state_t::SOCK_OK;

    TTRACE ( "Client: Connect to server. \n" );

    m_port      = portStr;
    m_conn_type = type;

    connect ( state );

    return ( state == sock_state_t::SOCK_OK );
}

void SocketClient::Close () {

    TTRACE ( "Client: Close connection. \n" );

    os_sockclose( m_sock );
    std::this_thread::sleep_for ( std::chrono::seconds ( 1 ) );
}

void SocketClient::connect ( sock_state_t& state ) {

    os_sockclose   ( m_sock );
    socket_open    ( state, m_conn_type, m_sock );
    socket_nodelay ( state, m_sock );
    socket_connect ( state, m_sock, m_conn_type, m_port );

    if ( state == sock_state_t::SOCK_OK ) {
        TTRACE ( "Client: Connected to sever. \n" );
    } else {
        TTRACE ( "Client: Failed to connect to server. \n" );
    }

    std::this_thread::sleep_for ( std::chrono::milliseconds (100) );
}

void SocketClient::SendPrefix ( sock_state_t& state, std::chrono::milliseconds delay_ms, sock_transaction_t& tr, const hid::types::storage_t& out_fame ) {

    if ( state == sock_state_t::SOCK_OK ) {
        try {

            hid::stream::params_t  params = {};

            params.command  =  hid::stream::cmd_t::STREAM_CMD_REQUEST;
            params.len      =  static_cast<uint32_t> ( out_fame.size() );
            hid::stream::Prefix::SetParams ( params, tr.out_hdr );
            hid::stream::Prefix::SetTimeout ( delay_ms, tr.out_hdr );

            frame_tx ( state, m_sock, tr.out_hdr );

            if ( state != sock_state_t::SOCK_OK ) {
                TTRACE ( "Client: Failed to Send Prefix. \n" );
            }

            if ( state == sock_state_t::SOCK_OK ) {
                tr.checkpoint_set (sock_checkpoint_type_t::CHECKPOINT_TX_HDR);
            }

        }   catch( ... ) {
            TTRACE ( "Client: Exception in Send Prefix. \n" );
            state = sock_state_t::SOCK_ERR_GENERAL;
        }
    }
}

void SocketClient::SendPayload ( sock_state_t& state, sock_transaction_t& tr, const hid::types::storage_t& out_fame ) {

    if ( state == sock_state_t::SOCK_OK ) {
        try {

            frame_tx ( state, m_sock, out_fame );

            if ( state != sock_state_t::SOCK_OK ) {
                TTRACE ( "Client: Failed to Send Payload. \n" );
            }

            if ( state == sock_state_t::SOCK_OK ) {
                tr.checkpoint_set (sock_checkpoint_type_t::CHECKPOINT_TX_PAYLOAD);
            }

        }   catch( ... ) {
            TTRACE ( "Client: Exception in Send Payload. \n" );
            state = sock_state_t::SOCK_ERR_GENERAL;
        }
    }
}

void SocketClient::RecvHeader ( sock_state_t& state, sock_transaction_t& tr ) {

    if ( state == sock_state_t::SOCK_OK ) {
        try {

            tr.inp_hdr.resize ( hid::stream::Prefix::PrefixSize() );

            frame_rx ( state, m_sock, tr.tv_expiration, tr.inp_hdr );

            if ( state == sock_state_t::SOCK_OK ) {
                if ( hid::stream::Prefix::Valid(tr.inp_hdr) ) {
                    hid::stream::params_t params;
                    hid::stream::Prefix::GetParams ( tr.inp_hdr, params );
                    tr.inp_pay.resize ( params.len );
                    tr.inp_cmd = static_cast<int> (params.command);
                    tr.checkpoint_set ( sock_checkpoint_type_t::CHECKPOINT_RX_HDR );
                } else {
                    TTRACE ( "Client: Wrong header received. \n" );
                    state = sock_state_t::SOCK_ERR_SYNC;
                }
            } else {
                TTRACE ( "Client: Failed to Receive Header. \n" );
            }

        } catch ( ... ) {
            TTRACE ( "Client: Exception in Recv Header. \n" );
            state = sock_state_t::SOCK_ERR_GENERAL;
        }
    }
}

void SocketClient::RecvPayload ( sock_state_t& state, sock_transaction_t& tr, hid::types::storage_t& in_frame ) {

    if ( state == sock_state_t::SOCK_OK ) {

        try {

            frame_rx ( state, m_sock, tr.tv_expiration, in_frame );

            if ( state == sock_state_t::SOCK_OK ) {
                tr.checkpoint_set ( sock_checkpoint_type_t::CHECKPOINT_RX_PAYLOAD );
            }
        }   catch( ... ) {
            TTRACE ( "Client: Exception in Recv Payload. \n" );
            state = sock_state_t::SOCK_ERR_GENERAL;
        }

    }
}

void SocketClient::LogTransaction ( const sock_transaction_t& tr, const sock_state_t conn_state ) {

    try {

        std::string log_msg;

        log_msg += tp_to_string  ( "Client:  ", tr.tv_start );
        log_msg += dur_to_string ( "snt HDR: ", tr.tv_start, tr.tv_snt_hdr );
        log_msg += dur_to_string ( "snt PAY: ", tr.tv_start, tr.tv_snt_pay );
        log_msg += dur_to_string ( "rcv HDR: ", tr.tv_start, tr.tv_rcv_hdr );
        log_msg += dur_to_string ( "rcv PAY: ", tr.tv_start, tr.tv_rcv_pay );
        log_msg += dur_to_string ( "Exp:     ", tr.tv_start, tr.tv_expiration );
        log_msg += "Status:  ";
        log_msg += std::to_string ( static_cast<uint32_t>(conn_state) );
        log_msg += "\r\n";
        log_msg += "\r\n";

        #if 1
            std::cout << log_msg;
        #else
            if ( conn_state == sock_state_t::SOCK_OK ) {
                printf ( "Client: Transaction done. \n" );
                fflush ( stdout );
            }
        #endif

    } catch( ... ) {
    }
}

bool SocketClient::Transaction ( std::chrono::milliseconds delay_ms, const hid::types::storage_t& out_fame, hid::types::storage_t& in_frame ) {

    sock_state_t state = sock_state_t::SOCK_OK;

    try {

        sock_transaction_t tr;

        in_frame.clear();

        tr.start    ( delay_ms );
        SendPrefix  ( state, delay_ms, tr, out_fame );
        SendPayload ( state, tr, out_fame );

        if ( state != sock_state_t::SOCK_OK ) {

            TTRACE ("Client: Attempt to reconnect to server. \n");

            state = sock_state_t::SOCK_OK;
            connect     ( state );

            SendPrefix  ( state, delay_ms, tr, out_fame );
            SendPayload ( state, tr, out_fame );
        }

        if ( state ==  sock_state_t::SOCK_OK ) {
            in_frame = std::move(tr.inp_pay);
        }

        RecvHeader  ( state, tr );
        RecvPayload ( state, tr, tr.inp_pay );

        LogTransaction (tr, state);

    }   catch( ... ) {
        TTRACE ( "Client: Exception in Transaction. \n" );
        state = sock_state_t::SOCK_ERR_GENERAL;
    }

    if ( state != sock_state_t::SOCK_OK ) {
        TTRACE ( "Client: Transaction failed. \n" );
        os_sockclose ( m_sock );
    }

    return  ( state == sock_state_t::SOCK_OK );
}

bool SocketClient::Transaction ( const hid::types::storage_t& out_fame, hid::types::storage_t& in_frame ) {
    return Transaction ( hid::socket::SOCK_COMM_TIMEOUT, out_fame, in_frame );
}

}
}
