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
           ~sock_transaction_t  ();
            sock_transaction_t  ( const sock_transaction_t& ref );
            sock_transaction_t  operator= ( const sock_transaction_t& ref ) = delete;

        public:
            void checkpoint_set ( sock_checkpoint_type_t point_type );
            void set_timeout    ( sock_duration_ms_t timeout_ms );
            void move_to_me     ( const sock_transaction_t& ref );
            bool is_expired     ();
            void close          ();

        public:
            ::hid::types::storage_t     inp_data;
            ::hid::types::storage_t     out_data;
            sock_transaction_id_t       m_idx;
            sock_socket_t               m_sock;

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

    typedef void (*ev_handler_t) (::hid::types::storage_t& in_data, ::hid::types::storage_t& out_data );

    typedef std::future<bool>  sock_thread_t;

    class SocketServer {

        public:
            SocketServer  ();
            ~SocketServer ();

        public:
            void  SetHandler ( ev_handler_t handler );

        public:
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

         // sock_conn_list_t    m_clients;

            std::string         m_port;
            conn_type_t         m_conn_type;
    };

    class SocketClient {

        public:
            SocketClient ();
           ~SocketClient ();

        public:
            bool Connect ( const char* const port, conn_type_t conn_type );
            bool Transaction ( std::chrono::milliseconds delayMs, const ::hid::types::storage_t& out_fame, ::hid::types::storage_t& in_frame );

        private:
            bool send_frame ( const ::hid::types::storage_t& out_frame, bool re_connect_enabled );
            bool recv_frame ( ::hid::types::storage_t& inp_frame );
            bool sock_tx ( const ::hid::types::storage_t& out_frame, size_t& tx_offset );
            void sock_close ();
            void sock_open ();
            bool sock_connect ();

        private:
            std::string   m_port;
            conn_type_t   m_conn_type;
            os_sock_t     m_sock;
    };


}

}

#endif

