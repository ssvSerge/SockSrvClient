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

    using sock_time_src_t        =  std::chrono::system_clock;
    using sock_duration_ms_t     =  std::chrono::milliseconds;
    using sock_checkpoint_t      =  std::chrono::time_point<sock_time_src_t>;
    using sock_transaction_id_t  =  size_t;
    using sock_socket_t          =  int;

    enum class sock_checkpoint_type_t {
        CHECKPOINT_UNKNOWN     =  0,
        CHECKPOINT_START       = 10,
        CHECKPOINT_RX_HDR      = 11,
        CHECKPOINT_RX_PAYLOAD  = 12,
        CHECKPOINT_TX_HDR      = 13,
        CHECKPOINT_TX_PAYLOAD  = 14,
        CHECKPOINT_COMMIT      = 15
    };

    enum class conn_type_t {
        CONN_TYPE_UNKNOW       =  0,
        CONN_TYPE_SOCK         = 10,
        CONN_TYPE_FILE         = 11,
    };

    class sock_transaction_t {

        public:
            sock_transaction_t  ();
            sock_transaction_t  ( const sock_transaction_t& ref ) = delete;
            sock_transaction_t  operator= ( const sock_transaction_t& ref ) = delete;

        public:
            void checkpoint_set ( sock_checkpoint_type_t point_type );
            void set_timeout    ( sock_duration_ms_t timeout_ms );
            bool is_expired     ();
            void close          ();

        public:
            sock_transaction_id_t       m_idx;

        private:
            bool                        m_closed;
            sock_checkpoint_t           m_start_time;
            sock_checkpoint_t           m_rcv_hdr;
            sock_checkpoint_t           m_rcv_payload;
            sock_checkpoint_t           m_snt_hdr;
            sock_checkpoint_t           m_snt_payload;
            sock_checkpoint_t           m_commit_time;
            sock_checkpoint_t           m_expiration_time;
    };

    typedef std::list<sock_transaction_t> sock_transaction_list_t;

    typedef void (*ev_handler_t) ( const ::hid::types::storage_t& in_data, ::hid::types::storage_t& out_data, int& error_code );

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
            bool  Shell   ( os_sock_t socket );

        private:
            bool  ConnProcessNew ( os_sock_t server_socket );
            bool  ConnMoveToExpired ();
            bool  ConnProcessExpired ();

        private:
            std::mutex          m_controller;

            std::atomic<bool>   m_stop;
            std::thread         m_server_thread;
            std::atomic<int>    m_instance_cnt;

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
            bool Transaction ( std::chrono::milliseconds delayMs, const ::hid::types::storage_t& out_fame, ::hid::types::storage_t& in_frame );

        private:
            std::string   m_port;
            conn_type_t   m_conn_type;
            os_sock_t     m_sock;
    };


}

}

#endif

