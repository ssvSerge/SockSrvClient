#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <atomic>

#include <SockServerClient.h>
#include <StreamPrefix.h>

#define  TTRACE              printf

using namespace std::chrono_literals;


namespace hid {
namespace transport {

//---------------------------------------------------------------------------//

constexpr int    SOCK_MAX_ERRORS_CNT    = 15;

//---------------------------------------------------------------------------//

uint32_t         SOCK_TEXT_TX_DELAY     = 0;

//---------------------------------------------------------------------------//

std::atomic<int> g_sockets_cnt(0);

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

    checkpoint_t curr_time = time_source_t::now ();

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
    IN    MANDATORY const checkpoint_t& time
) {

    std::time_t  tt   = time_source_t::to_time_t (time);
    std::tm      tm   = *std::gmtime(&tt);
    std::stringstream ss;

    ss << std::put_time( &tm, "UTC: %Y-%m-%d %H:%M:%S" );
    return ss.str();
}

static std::string dur_to_string ( 
    IN    MANDATORY const checkpoint_t& start,
    IN    MANDATORY const checkpoint_t& end
) {

    std::string ret_val;

    uint64_t seconds_cnt = std::chrono::duration_cast<std::chrono::seconds>(end.time_since_epoch()).count();
    if ( seconds_cnt > 0 ) {

        std::string prefix = "+";
        auto diff = (end - start);

        if ( end < start ) {
            diff = (start - end);
            prefix = "-";
        }

        duration_ms_t diff_ms;
        diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff);

        ret_val += "(";
        ret_val += prefix;
        ret_val += std::to_string ( diff_ms.count() );
        ret_val += "ms)";

    }

    return ret_val;
}

//---------------------------------------------------------------------------//

static void socket_open ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY conn_type_t         conn_type, 
    INOUT MANDATORY os_sock_t&          sock 
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        int  af_mode = AF_UNSPEC;
        if ( conn_type == conn_type_t::CONN_TYPE_SOCK ) {
            af_mode = AF_INET;
        } else
        if ( conn_type == conn_type_t::CONN_TYPE_FILE ) {
            af_mode = AF_UNIX;
        }

        conn_state = conn_state_t::CONN_ERR_OPEN;

        sock = ::socket ( af_mode, SOCK_STREAM, 0 );
        if ( sock_valid (sock) ) {
            g_sockets_cnt++;
            os_sock_nodelay ( sock );
            conn_state = conn_state_t::CONN_OK;
        }

    }
}

static void socket_close ( 
    INOUT MANDATORY os_sock_t& sock
) {

    if ( sock != static_cast<os_sock_t> (SOCK_INVALID_SOCK) ) {
        g_sockets_cnt--;
        os_sockclose( sock );
    }
}

static void socket_nodelay ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    INOUT MANDATORY os_sock_t&          sock 
) {

    if ( conn_state == conn_state_t::CONN_OK ) {
        os_sock_nodelay ( sock );
    }
}

static void socket_bind ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY conn_type_t         conn_type, 
    IN    MANDATORY const char* const   port_str 
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

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

        conn_state = conn_state_t::CONN_ERR_BIND;

        int io_res = ::bind ( sock, addr_ptr, addr_size );
        if ( io_res != -1 ) {
            conn_state = conn_state_t::CONN_OK;
        }
    }
    
}

static void socket_listen ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock 
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        conn_state = conn_state_t::CONN_ERR_LISTEN;

        int listen_res = ::listen ( sock, SOMAXCONN );
        if ( listen_res == 0 ) {
            conn_state = conn_state_t::CONN_OK;
        }

    }
}

static void socket_wait ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY duration_ms_t       timeout_ns
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        struct timeval  timeout;
        fd_set          readfds;

        FD_ZERO ( &readfds );
        FD_SET ( sock, &readfds );

        std::chrono::microseconds mks = std::chrono::duration_cast<std::chrono::microseconds>(timeout_ns);

        timeout.tv_sec = static_cast<long> (mks.count () / 1000000);
        timeout.tv_usec = static_cast<long> (mks.count () % 1000000);

        int select_res = select ( static_cast<int>(sock + 1), &readfds, NULL, NULL, &timeout );

        conn_state = conn_state_t::CONN_ERR_SELECT;

        if ( select_res > 0 ) {
            conn_state = conn_state_t::CONN_OK;
        } else
        if ( select_res == 0 ) {
            conn_state = conn_state_t::CONN_ERR_TIMEOUT;
        } else {
            int select_err = sock_error ();
            if ( (select_err == EINTR) || (select_err == EWOULDBLOCK) ) {
                conn_state = conn_state_t::CONN_ERR_TIMEOUT;
            } else {
                conn_state = conn_state_t::CONN_ERR_SELECT;
            }
        }
    }
}

static void socket_accept ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           server_sock, 
    OUT   MANDATORY os_sock_t&          client_sock
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        conn_state = conn_state_t::CONN_ERR_ACCEPT;

        client_sock = ::accept ( server_sock, NULL, NULL );

        if ( sock_valid(client_sock) ) {
            g_sockets_cnt++;
            conn_state = conn_state_t::CONN_OK;
        }
    }
}

static void socket_connect ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY conn_type_t         conn_type, 
    IN    MANDATORY const std::string   port 
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

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

        conn_state = conn_state_t::CONN_ERR_CONNECT;

        if ( con_addr_ptr != nullptr ) {
            int io_res = ::connect ( sock, con_addr_ptr, con_addr_len );
            if ( io_res == 0 ) {
                conn_state = conn_state_t::CONN_OK;
            }
        }

    }
}

static void frame_wait ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        struct timeval tv;
        fd_set readfds;

        FD_ZERO ( &readfds );
        FD_SET ( sock, &readfds );

        tv.tv_sec  = 1;
        tv.tv_usec = 0;

        int io_res = select ( static_cast<int>(sock + 1), &readfds, NULL, NULL, &tv );

        if ( io_res < 0 ) {
            TTRACE ("Socket error: SELECT failed. \n");
            conn_state = conn_state_t::CONN_ERR_SELECT;
        } else
        if ( io_res == 0 ) {
            conn_state = conn_state_t::CONN_ERR_TIMEOUT;
        }
    }
}

static void frame_tx ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY const hid::types::storage_t& out_frame 
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

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
                    conn_state = conn_state_t::CONN_ERR_TX;
                    break;
                }

                if ( io_res == 0 ) {
                    conn_state = conn_state_t::CONN_ERR_CLOSED;
                    break;
                }

                tx_cnt += io_res;
            }

        }

    }
}

static void frame_rx ( 
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock, 
    OUT   MANDATORY hid::types::storage_t& inp_frame,
    IN    MANDATORY duration_ms_t       delay
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        if ( inp_frame.size () > 0 ) {

            checkpoint_t end_time = time_source_t::now() + delay;

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
                    conn_state = conn_state_t::CONN_ERR_CLOSED;
                    break;
                }

                io_res = sock_error();
                if ( io_res == EAGAIN ) {
                    continue;
                }

                if ( io_res != EWOULDBLOCK ) {
                    conn_state = conn_state_t::CONN_ERR_RX;
                    break;
                }

                struct timeval tv;
                fd_set readfds;

                if ( ! get_timeval ( end_time, tv ) ) {
                    conn_state = conn_state_t::CONN_ERR_TIMEOUT;
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
    INOUT MANDATORY conn_state_t&       conn_state,
    IN    MANDATORY os_sock_t           sock, 
    IN    MANDATORY checkpoint_t        exptiration_time,
    OUT   MANDATORY hid::types::storage_t& inp_frame 
) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        checkpoint_t curr_time = time_source_t::now ();

        if ( curr_time >= exptiration_time ) {
            conn_state = conn_state_t::CONN_ERR_TIMEOUT;
        } else {
            duration_ms_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(exptiration_time - curr_time);
            frame_rx ( conn_state, sock, inp_frame, ms );
        }
    }
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

void SocketServer::StartClient ( conn_state_t& conn_state, os_sock_t client_sock ) {

    if ( conn_state == conn_state_t::CONN_OK ) {
        sock_thread_t client_handler;
        client_handler = std::async ( std::launch::async, &SocketServer::Shell, this, client_sock );
        m_clients.emplace_back ( std::move ( client_handler ) );
    }
}

void SocketServer::Service () {

    os_sock_t server_sock = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
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

        if ( ! sock_valid(server_sock) ) {

            conn_state_t init_res = conn_state_t::CONN_OK;
            socket_open   ( init_res, m_conn_type, server_sock );
            socket_bind   ( init_res, server_sock, m_conn_type, m_port.c_str() );
            socket_listen ( init_res, server_sock );

            if ( init_res != conn_state_t::CONN_OK ) {
            	std::string err_msg;
            	err_msg  = "Failed to start. ";
                if ( m_conn_type == conn_type_t::CONN_TYPE_FILE ) {
                    err_msg += " On file: "; 
                } else {
                    err_msg += " On socket: ";
                }
                err_msg += m_port;
                err_msg += "\r\n";

                TTRACE ( "Server: %s", err_msg.c_str() );
                err_cnt_start++;
                socket_close (server_sock);
                std::this_thread::sleep_for ( 1s );
                continue;
            }

        }

        err_cnt_start = 0;

        {   // Accept new connection(s)
            os_sock_t client_sock = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
            conn_state_t conn_res = conn_state_t::CONN_OK;
            m_instance_active = true;

            if ( to_log ) {
                to_log = false;
                TTRACE ( "Server: Ready to accept new connections. \n" );
            }

            socket_wait   ( conn_res, server_sock, std::chrono::seconds(1) );
            socket_accept ( conn_res, server_sock, client_sock );
            StartClient   ( conn_res, client_sock );

            if ( conn_res == conn_state_t::CONN_OK ) {
                continue;
            } else
            if ( conn_res != conn_state_t::CONN_ERR_TIMEOUT ) {
                TTRACE ( "Server: Failed to handle new connections. \n" );
                break;
            }

        }

    }

    socket_close ( server_sock );

    if ( m_conn_type == conn_type_t::CONN_TYPE_FILE ) {
        sock_unlink ( m_port.c_str() );
    }

    TTRACE ( "Server: Down. \n" );

    m_instance_active = false;
}

bool SocketServer::StartMe ( const char* const port, conn_type_t conn_type ) {

    bool ret_val = false;

    TTRACE ( "Server: Start requested. \n" );

    try {

        if ( ! m_instance_active ) {

            m_stop = false;

            m_port = port;
            m_conn_type = conn_type;

            m_server_thread = std::thread ( &SocketServer::Service, this );

            while ( ! m_instance_active ) {
                std::this_thread::sleep_for ( std::chrono::milliseconds (10) );
            }

            ret_val = true;

        } else {
            TTRACE ( "Server: Already running. \n" );
        }
            
    } catch ( ... ) {
        TTRACE ("Server: Exception in Server Start. \n");
    }

    return ret_val;
}

void SocketServer::StopMe ( void ) {

    TTRACE ( "Server: Stop requested. \n" );

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

void SocketServer::ShellCmdStart ( os_sock_t sock, conn_state_t& conn_state, transaction_t& tr ) {

    UNUSED (sock);

    try {
        tr.start ( COMM_TIMEOUT );
        tr.inp_hdr.resize ( hid::stream::Prefix::PrefixSize() );
    } catch ( ... ) {
        TTRACE ( "Server: Exception in Cmd Start. \n" );
        conn_state = conn_state_t::CONN_ERR_GENERAL;
    }
}

void SocketServer::ShellReadPrefix ( os_sock_t sock, conn_state_t& conn_state, transaction_t& tr ) {
    if ( conn_state == conn_state_t::CONN_OK ) {
        try {

            frame_rx ( conn_state, sock, tr.tv_expiration, tr.inp_hdr );
            if ( conn_state == conn_state_t::CONN_OK ) {
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
                    tr.inp_code = params.code;

                    tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_RX_HDR );
                } else {
                    TTRACE ( "Server: Wrong prefix received. \n" );
                    conn_state = conn_state_t::CONN_ERR_SYNC;
                }
            } else {
                TTRACE ("Server: Failed to Read Prefix. \n");
            }

        } catch ( ... ) {
            TTRACE ( "Server: Exception in Read Prefix. \n" );
            conn_state = conn_state_t::CONN_ERR_GENERAL;
        }
    }
}

void SocketServer::ShellReadPayload ( os_sock_t sock, conn_state_t& conn_state, transaction_t& tr ) {
    if ( conn_state == conn_state_t::CONN_OK ) {
        try {
            frame_rx ( conn_state, sock, tr.tv_expiration, tr.inp_pay );
            tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_RX_PAYLOAD );
        } catch ( ... ) {
            TTRACE ( "Server: Exception in Read Payload. \n" );
            conn_state = conn_state_t::CONN_ERR_GENERAL;
        }
    }
}

void SocketServer::ShellCmdExec ( os_sock_t sock, conn_state_t& conn_state, transaction_t& tr ) {

    UNUSED (sock);

    if ( conn_state == conn_state_t::CONN_OK ) {
        
        try {

            {   // Exec command
                hid::stream::cmd_t inp_cmd = static_cast<hid::stream::cmd_t> (tr.inp_cmd);

                if ( inp_cmd == hid::stream::cmd_t::STREAM_CMD_PING_REQUEST ) {
                    tr.out_cmd  = static_cast<uint32_t> (hid::stream::cmd_t::STREAM_CMD_PING_RESPONSE);
                    tr.out_code = tr.inp_code;
                    tr.out_pay.clear ();
                } else 
                if ( inp_cmd == hid::stream::cmd_t::STREAM_CMD_REQUEST ) {
                    tr.out_cmd = static_cast<uint32_t> (hid::stream::cmd_t::STREAM_CMD_RESPONSE);
                    (m_ev_handler) ( tr.inp_pay, tr.out_pay, tr.out_code );
                }
            }

            tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_EXEC );

            {   // Format response.
                hid::stream::params_t params;

                params.command = static_cast<hid::stream::cmd_t> (tr.out_cmd);
                params.code    = tr.out_code;
                params.len     = static_cast<uint32_t> (tr.out_pay.size() );

                hid::stream::Prefix::SetParams ( params, tr.out_hdr );
            }

        } catch( ... ) {
            TTRACE ( "Server: Exception in Exec command. \n" );
            conn_state = conn_state_t::CONN_ERR_EXEC;
        }


    }
}

void SocketServer::ShellSendPrefix ( os_sock_t sock, conn_state_t& conn_state, transaction_t& tr ) {
    if ( conn_state == conn_state_t::CONN_OK ) {
        frame_tx ( conn_state, sock, tr.out_hdr );
        tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_TX_HDR );
        if ( conn_state != conn_state_t::CONN_OK ) {
            TTRACE ( "Server: Failed to send prefix. \n" );
        }
    }
}

void SocketServer::ShellSendPayload ( os_sock_t sock, conn_state_t& conn_state, transaction_t& tr ) {
    if ( conn_state == conn_state_t::CONN_OK ) {
        frame_tx ( conn_state, sock, tr.out_pay );
        tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_TX_PAYLOAD );
        if ( conn_state != conn_state_t::CONN_OK ) {
            TTRACE ( "Server: Failed to send payload. \n" );
        }
    }
}

void SocketServer::LogTransaction ( const transaction_t& tr, const conn_state_t conn_state ) {

    try {

        std::string log_msg;
        checkpoint_t max_time;

        log_msg += "Server: Transaction ";
        log_msg += tp_to_string  ( tr.tv_start );
        if ( max_time < tr.tv_start ) {
            max_time = tr.tv_start;
        }

        log_msg += "; RCV HDR: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_rcv_hdr );
        if ( max_time < tr.tv_rcv_hdr ) {
            max_time = tr.tv_rcv_hdr;
        }

        log_msg += "; RCV PAY: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_rcv_pay );
        if ( max_time < tr.tv_rcv_pay ) {
            max_time = tr.tv_rcv_pay;
        }

        log_msg += "; SNT HDR: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_snt_hdr );
        if ( max_time < tr.tv_snt_hdr ) {
            max_time = tr.tv_snt_hdr;
        }

        log_msg += "; SNT PAY: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_snt_pay );
        if ( max_time < tr.tv_snt_pay ) {
            max_time = tr.tv_snt_pay;
        }

        log_msg += "; Exp: ";
        log_msg += dur_to_string ( max_time, tr.tv_expiration );

        log_msg += "; Status: ";
        log_msg += std::to_string ( static_cast<uint32_t>(conn_state) );

        log_msg += "\r\n";

        #if 0
            std::cout << log_msg;
        #endif

    } catch ( ... ) {
    }
}

void SocketServer::ShellClose ( os_sock_t sock, const conn_state_t& conn_state ) {

    if ( sock_valid (sock) ) {

        if ( conn_state != conn_state_t::CONN_ERR_CLOSED ) {

            hid::stream::params_t   params;
            hid::types::storage_t   out_frame;

            params.command = hid::stream::cmd_t::STREAM_CMD_ERROR;
            params.code    = static_cast<uint32_t> (conn_state);
            params.len     = 0;

            hid::stream::Prefix::SetParams ( params, out_frame );

            conn_state_t resp = conn_state_t::CONN_OK;
        
            frame_tx ( resp, sock, out_frame );
            if ( resp == conn_state_t::CONN_OK ) {
                TTRACE ( "Server: Sent CMD_ERROR. \n" );
            }
        }

    }
}

bool SocketServer::Shell ( os_sock_t sock ) {

    bool ret_val = false;

    TTRACE ("Server: New client connected. \n");

    try {

        transaction_t tr;

        for ( ; ; ) {

            if ( m_stop ) {
                TTRACE ("Server: Command STOP received. \n");
                ret_val = true;
                break;
            }

            conn_state_t conn_state = conn_state_t::CONN_OK;

            frame_wait ( conn_state, sock );
            if ( conn_state == conn_state_t::CONN_ERR_TIMEOUT ) {
                continue;
            }

            if ( conn_state != conn_state_t::CONN_OK ) {
                TTRACE ( "Server: Shell failed. \n" );
                ret_val = false;
                break;
            }

            ShellCmdStart    ( sock, conn_state, tr );
            ShellReadPrefix  ( sock, conn_state, tr );
            ShellReadPayload ( sock, conn_state, tr );
            ShellCmdExec     ( sock, conn_state, tr );
            ShellSendPrefix  ( sock, conn_state, tr );
            ShellSendPayload ( sock, conn_state, tr );
            LogTransaction   ( tr, conn_state );

            tr.reset();

            if ( conn_state != conn_state_t::CONN_OK ) {
                ShellClose (sock, conn_state);
                break;
            }

        }


    } catch ( ... ) {
        TTRACE ( "Server: Exception in Shell. \n" );
    }

    socket_close ( sock );
    TTRACE ( "Server: Client shell closed. \n" );

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
        socket_close (m_sock);
        m_sock = static_cast<os_sock_t> (SOCK_INVALID_SOCK);
    }

}

bool SocketClient::Connect ( const char* const portStr, conn_type_t type ) {

    conn_state_t state = conn_state_t::CONN_OK;

    TTRACE ( "Client: Connect to server. \n" );

    m_port      = portStr;
    m_conn_type = type;

    connect ( state );

    return ( state == conn_state_t::CONN_OK);
}

void SocketClient::Close () {

    TTRACE ( "Client: Close connection. \n" );
    socket_close ( m_sock );
}

void SocketClient::connect ( conn_state_t& conn_state ) {

    socket_close   ( m_sock );
    socket_open    ( conn_state, m_conn_type, m_sock );
    socket_connect ( conn_state, m_sock, m_conn_type, m_port );
    socket_nodelay ( conn_state, m_sock );

    if ( conn_state == conn_state_t::CONN_OK ) {
        TTRACE ( "Client: Connected to sever. \n" );
    } else {
        TTRACE ( "Client: Failed to connect to server. \n" );
    }

    std::this_thread::sleep_for ( std::chrono::milliseconds (100) );
}

void SocketClient::SendPrefix ( conn_state_t& conn_state, duration_ms_t delay_ms, transaction_t& tr, size_t out_fame_len ) {

    if ( conn_state == conn_state_t::CONN_OK ) {
        try {

            hid::stream::params_t  params = {};

            params.command  =  hid::stream::cmd_t::STREAM_CMD_REQUEST;
            params.len      =  static_cast<uint32_t> (out_fame_len);
            hid::stream::Prefix::SetParams ( params, tr.out_hdr );
            hid::stream::Prefix::SetTimeout ( delay_ms, tr.out_hdr );

            frame_tx ( conn_state, m_sock, tr.out_hdr );
            tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_TX_HDR );

            if ( conn_state != conn_state_t::CONN_OK ) {
                TTRACE ( "Client: Failed to Send Prefix. \n" );
            }

        }   catch( ... ) {
            TTRACE ( "Client: Exception in Send Prefix. \n" );
            conn_state = conn_state_t::CONN_ERR_GENERAL;
        }
    }
}

void SocketClient::SendPayload ( conn_state_t& conn_state, transaction_t& tr, const hid::types::storage_t& out_fame ) {

    if ( conn_state == conn_state_t::CONN_OK ) {
        try {

            frame_tx ( conn_state, m_sock, out_fame );
            tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_TX_PAYLOAD );

            if ( conn_state != conn_state_t::CONN_OK ) {
                TTRACE ( "Client: Failed to Send Payload. \n" );
            }

        }   catch( ... ) {
            TTRACE ( "Client: Exception in Send Payload. \n" );
            conn_state = conn_state_t::CONN_ERR_GENERAL;
        }
    }
}

void SocketClient::RecvHeader ( conn_state_t& conn_state, transaction_t& tr ) {

    if ( conn_state == conn_state_t::CONN_OK ) {
        try {

            tr.inp_hdr.resize ( hid::stream::Prefix::PrefixSize() );

            frame_rx ( conn_state, m_sock, tr.tv_expiration, tr.inp_hdr );
            tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_RX_HDR );

            if ( conn_state == conn_state_t::CONN_OK ) {
                if ( hid::stream::Prefix::Valid(tr.inp_hdr) ) {
                    hid::stream::params_t params;
                    hid::stream::Prefix::GetParams ( tr.inp_hdr, params );
                    tr.out_cmd  = static_cast<uint32_t> (params.command);
                    tr.out_code = params.code;
                    tr.inp_pay.resize ( params.len );
                    tr.inp_cmd  = static_cast<int> (params.command);
                } else {
                    TTRACE ( "Client: Wrong header received. \n" );
                    conn_state = conn_state_t::CONN_ERR_SYNC;
                }
            } else {
                TTRACE ( "Client: Failed to Receive Header. \n" );
            }

        } catch ( ... ) {
            TTRACE ( "Client: Exception in Recv Header. \n" );
            conn_state = conn_state_t::CONN_ERR_GENERAL;
        }
    }
}

void SocketClient::RecvPayload ( conn_state_t& conn_state, transaction_t& tr, hid::types::storage_t& in_frame ) {

    if ( conn_state == conn_state_t::CONN_OK ) {

        try {

            frame_rx ( conn_state, m_sock, tr.tv_expiration, in_frame );
            tr.checkpoint_set ( checkpoint_id_t::CHECKPOINT_RX_PAYLOAD );

            if ( conn_state != conn_state_t::CONN_OK ) {
                TTRACE ( "Client: Failed to receive payload. \n" );
            }

        }   catch( ... ) {
            TTRACE ( "Client: Exception in Recv Payload. \n" );
            conn_state = conn_state_t::CONN_ERR_GENERAL;
        }

    }
}

void SocketClient::LogTransaction ( const transaction_t& tr, const conn_state_t conn_state ) {

    try {

        std::string  log_msg;
        checkpoint_t max_time;

        log_msg += "Client: Transaction ";
        log_msg += tp_to_string ( tr.tv_start );
        max_time = tr.tv_start;

        log_msg += "; SNT HDR: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_snt_hdr );
        if ( max_time < tr.tv_snt_hdr ) {
            max_time = tr.tv_snt_hdr;
        }

        log_msg += "; SNT PAY: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_snt_pay );
        if ( max_time < tr.tv_snt_pay ) {
            max_time = tr.tv_snt_pay;
        }

        log_msg += "; RCV HDR: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_rcv_hdr );
        if ( max_time < tr.tv_rcv_hdr ) {
            max_time = tr.tv_rcv_hdr;
        }

        log_msg += "; RCV PAY: ";
        log_msg += dur_to_string ( tr.tv_start, tr.tv_rcv_pay );
        if ( max_time < tr.tv_rcv_pay ) {
            max_time = tr.tv_rcv_pay;
        }

        log_msg += "; Exp: ";
        log_msg += dur_to_string ( max_time, tr.tv_expiration );

        log_msg += "; Status: ";
        log_msg += std::to_string ( static_cast<uint32_t>(conn_state) );

        log_msg += "\r\n";

        #if 0
            std::cout << log_msg;
        #endif

    } catch( ... ) {
    }
}

bool SocketClient::TransactionInt ( conn_state_t& conn_state, duration_ms_t delay_ms, transaction_t& tr, const hid::types::storage_t& out_frame ) {

    bool ret_val = false;

    try {

        SendPrefix  ( conn_state, delay_ms, tr, out_frame.size() );
        SendPayload ( conn_state, tr, out_frame );

        RecvHeader  ( conn_state, tr );
        RecvPayload ( conn_state, tr, tr.inp_pay );

        ret_val = true;

    } catch ( ... ) {
        TTRACE ( "Client: Exception in TransactionInt. \n" );
    }

    return ret_val;
}

bool SocketClient::Transaction ( duration_ms_t delay_ms, const hid::types::storage_t& out_frame, hid::types::storage_t& in_frame, uint32_t& in_code ) {

    bool ret_val = false;

    try {

        transaction_t tr;
        conn_state_t state = conn_state_t::CONN_OK;

        in_frame.clear ();
        in_code = 0;

        tr.start ( delay_ms );

        TransactionInt ( state, delay_ms, tr, out_frame );
        if ( (state == conn_state_t::CONN_ERR_TX) || (state == conn_state_t::CONN_ERR_RX) ) {
            state = conn_state_t::CONN_OK;
            connect ( state );
            TransactionInt ( state, delay_ms, tr, out_frame );
        }

        LogTransaction ( tr, state );

        if ( state == conn_state_t::CONN_OK ) {
            in_frame = std::move ( tr.inp_pay );
            in_code  = tr.out_code;
        } else {
            TTRACE ( "Client: Close connection; Attempt to re-start. \n" );
            socket_close ( m_sock );
        }

        if ( state == conn_state_t::CONN_OK ) {
            ret_val = true;
        }

    }   catch( ... ) {
        TTRACE ( "Client: Exception in Transaction. \n" );
    }

    return ret_val;
}

bool SocketClient::Sync ( duration_ms_t delay_ms, uint32_t rand ) {

    bool ret_val = false;

    try {

        hid::types::storage_t   request     = {};
        hid::types::storage_t   response    = {};
        hid::stream::params_t   params_out  = {};
        hid::stream::params_t   params_inp  = {};
        conn_state_t            state       = conn_state_t::CONN_OK;

        params_out.command = hid::stream::cmd_t::STREAM_CMD_PING_REQUEST;
        params_out.code    = rand;
        params_out.len     = 0;

        hid::stream::Prefix::SetParams  ( params_out, request );
        hid::stream::Prefix::SetTimeout ( delay_ms,   request );

        response.resize ( hid::stream::Prefix::PrefixSize() );

        frame_tx ( state, m_sock, request );
        frame_rx ( state, m_sock, response, delay_ms );

        if ( state != conn_state_t::CONN_OK ) {
            TTRACE ( "Client: Failed to Read/Write. \n" );
        } else
        if ( ! hid::stream::Prefix::GetParams ( response, params_inp ) ) {
            TTRACE ( "Client: Wrong prefix received. \n" );
        } else
        if ( params_inp.command != hid::stream::cmd_t::STREAM_CMD_PING_RESPONSE ) {
            TTRACE ( "Client: Wrong COMMAND received; PING_RESPONSE expected. \n" );
        } else 
        if ( params_inp.code != params_out.code ) {
            TTRACE ( "Client: Wrong CODE received. \n" );
        } else {
            ret_val = true;
        }

    } catch( ... ) {
        TTRACE ( "Client: Exception in Sync. \n" );
    }

    return ret_val;
}

//---------------------------------------------------------------------------//

}
}
