#ifndef __SOCKSERVERCLIENT_H__
#define __SOCKSERVERCLIENT_H__

#include <chrono>
#include <list>
#include <future>
#include <thread>
#include <atomic>

#include <HidTypes.h>
#include <HidOsTypes.h>

namespace hid {

namespace socket {

    using time_source_t = std::chrono::system_clock;
    using duration_ns_t = std::chrono::nanoseconds;
    using duration_us_t = std::chrono::microseconds;
    using duration_ms_t = std::chrono::milliseconds;
    using checkpoint_t  = std::chrono::time_point<time_source_t>;

    constexpr auto SOCK_COMM_TIMEOUT = std::chrono::milliseconds ( 5 * 1000 );

    enum class sock_checkpoint_type_t {
        CHECKPOINT_UNKNOWN      =  0,
        CHECKPOINT_START        = 10,
        CHECKPOINT_RX_HDR       = 11,
        CHECKPOINT_RX_PAYLOAD   = 12,
        CHECKPOINT_EXEC         = 13,
        CHECKPOINT_TX_HDR       = 14,
        CHECKPOINT_TX_PAYLOAD   = 15
    };

    enum class conn_type_t {
        CONN_TYPE_UNKNOW        =  0,
        CONN_TYPE_SOCK          = 10,
        CONN_TYPE_FILE          = 11,
    };

    enum class sock_state_t {
        SOCK_STATE_UNKNOWN      =   0,
        SOCK_OK                 = 100,
        SOCK_RX_DONE            = 200,
        SOCK_TX_DONE            = 300,
        SOCK_ERR_GENERAL        = 400,
        SOCK_ERR_CLOSED         = 401,
        SOCK_ERR_TIMEOUT        = 402,
        SOCK_ERR_OPEN           = 403,
        SOCK_ERR_BIND           = 404,
        SOCK_ERR_LISTEN         = 405,
        SOCK_ERR_SELECT         = 406,
        SOCK_ERR_ACCEPT         = 407,
        SOCK_ERR_CONNECT        = 408,
        SOCK_ERR_RX             = 409,
        SOCK_ERR_TX             = 410,
        SOCK_ERR_EXEC           = 411,
        SOCK_ERR_SYNC           = 500
    };

    class sock_transaction_t {

        public:
            sock_transaction_t  ();
            sock_transaction_t  ( const sock_transaction_t& ref ) = delete;
            sock_transaction_t  operator= ( const sock_transaction_t& ref ) = delete;

        public:
            void start ( duration_ms_t expiration_default_ms );
            void expiration_set ( duration_ms_t expiration_ms );
            void checkpoint_set ( sock_checkpoint_type_t point_type );
            void reset ( void );

        public:
            int                     inp_cmd;
            hid::types::storage_t   inp_hdr;
            hid::types::storage_t   inp_pay;
            hid::types::storage_t   out_hdr;
            hid::types::storage_t   out_pay;

        public:
            checkpoint_t    tv_start;
            checkpoint_t    tv_rcv_hdr;
            checkpoint_t    tv_rcv_pay;
            checkpoint_t    tv_exec;
            checkpoint_t    tv_snt_hdr;
            checkpoint_t    tv_snt_pay;
            checkpoint_t    tv_expiration;
    };

    typedef std::list<sock_transaction_t> sock_transaction_list_t;

    typedef void (*ev_handler_t) ( const hid::types::storage_t& in_data, hid::types::storage_t& out_data, uint32_t& error_code );

    typedef std::future<bool>           sock_thread_t;
    typedef std::list<sock_thread_t>    clients_list_t;

    class SocketServer {

        public:
            SocketServer  ();
            ~SocketServer ();

        public:
            void  SetHandler ( ev_handler_t handler );
            bool  Start ( const char* const port, conn_type_t conn_type );
            void  Stop  ( void );

        private:
            void  Service ( );

        private:
            bool  Shell            ( os_sock_t socket );
            void  ShellCmdStart    ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr );
            void  ShellReadPrefix  ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr );
            void  ShellReadPayload ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr );
            void  ShellCmdExec     ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr );
            void  ShellSendPrefix  ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr );
            void  ShellSendPayload ( os_sock_t socket, sock_state_t& state, sock_transaction_t& tr );
            void  ShellClose       ( os_sock_t socket, const sock_state_t& state );
            void  LogTransaction   ( const sock_transaction_t& tr, const sock_state_t conn_state );

        private:
            void  StartClient      ( sock_state_t& sock_state, os_sock_t client_sock );

        private:
            std::mutex          m_controller;

            std::atomic<bool>   m_stop;
            std::thread         m_server_thread;
            std::atomic<bool>   m_instance_active;

            clients_list_t      m_clients;

            std::string         m_port;
            conn_type_t         m_conn_type;

            ev_handler_t        m_ev_handler;
    };

    class SocketClient {

        public:
            SocketClient ();
           ~SocketClient ();

        public:
            bool Connect ( const char* const port, conn_type_t conn_type );
            void Close ();
            bool Transaction ( std::chrono::milliseconds delayMs, const hid::types::storage_t& out_fame, hid::types::storage_t& in_frame );
            bool Transaction ( const hid::types::storage_t& out_fame, hid::types::storage_t& in_frame );

        private:
            void connect        ( sock_state_t& state );
            void SendPrefix     ( sock_state_t& state, std::chrono::milliseconds delay_ms, sock_transaction_t& tr, const hid::types::storage_t& out_fame );
            void SendPayload    ( sock_state_t& state, sock_transaction_t& tr, const hid::types::storage_t& out_fame );
            void RecvHeader     ( sock_state_t& state, sock_transaction_t& tr );
            void RecvPayload    ( sock_state_t& state, sock_transaction_t& tr, hid::types::storage_t& in_frame );
            void LogTransaction ( const sock_transaction_t& tr, const sock_state_t conn_state );


        private:
            std::string   m_port;
            conn_type_t   m_conn_type;
            os_sock_t     m_sock;
    };

}

}

#endif

