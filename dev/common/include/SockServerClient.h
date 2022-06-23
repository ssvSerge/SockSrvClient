#ifndef __SOCKSERVERCLIENT_H__
#define __SOCKSERVERCLIENT_H__

#include <chrono>
#include <list>
#include <future>
#include <thread>
#include <atomic>

#include <HidTypes.h>
#include <HidOsTypes.h>
#include <HidTransport.h>

namespace hid {

namespace transport {


    class sock_transaction_t {

        public:
            sock_transaction_t  ();
            sock_transaction_t  ( const sock_transaction_t& ref ) = delete;
            sock_transaction_t  operator= ( const sock_transaction_t& ref ) = delete;

        public:
            void start ( duration_ms_t expiration_default_ms );
            void expiration_set ( duration_ms_t expiration_ms );
            void checkpoint_set ( checkpoint_id_t point_type );
            void reset ( void );

        public:
            int                     inp_cmd;
            int                     inp_code;
            int                     out_cmd;
            int                     out_code;
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
           ~SocketServer  ();

        public:
            void  SetHandler ( ev_handler_t handler );
            bool  Start ( const char* const port, conn_type_t conn_type );
            void  Stop  ( void );

        private:
            void  Service ( );

        private:
            bool  Shell            ( os_sock_t socket );
            void  ShellCmdStart    ( os_sock_t socket, conn_state_t& state, sock_transaction_t& tr );
            void  ShellReadPrefix  ( os_sock_t socket, conn_state_t& state, sock_transaction_t& tr );
            void  ShellReadPayload ( os_sock_t socket, conn_state_t& state, sock_transaction_t& tr );
            void  ShellCmdExec     ( os_sock_t socket, conn_state_t& state, sock_transaction_t& tr );
            void  ShellSendPrefix  ( os_sock_t socket, conn_state_t& state, sock_transaction_t& tr );
            void  ShellSendPayload ( os_sock_t socket, conn_state_t& state, sock_transaction_t& tr );
            void  ShellClose       ( os_sock_t socket, const conn_state_t& state );
            void  LogTransaction   ( const sock_transaction_t& tr, const conn_state_t state );

        private:
            void  StartClient      ( conn_state_t& state, os_sock_t client_sock );

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
            bool Transaction ( std::chrono::milliseconds delayMs, const hid::types::storage_t& out_frame, hid::types::storage_t& in_frame, uint32_t& in_code );
            bool Transaction ( const hid::types::storage_t& out_frame, hid::types::storage_t& in_frame, uint32_t& in_code );

        private:
            void connect        ( conn_state_t& state );
            void SendPrefix     ( conn_state_t& state, std::chrono::milliseconds delay_ms, sock_transaction_t& tr, size_t out_fame_len );
            void SendPayload    ( conn_state_t& state, sock_transaction_t& tr, const hid::types::storage_t& out_fame );
            void RecvHeader     ( conn_state_t& state, sock_transaction_t& tr );
            void RecvPayload    ( conn_state_t& state, sock_transaction_t& tr, hid::types::storage_t& in_frame );
            void LogTransaction ( const sock_transaction_t& tr, const conn_state_t conn_state );
            bool TransactionInt ( conn_state_t& state, std::chrono::milliseconds delay_ms, sock_transaction_t& tr, const hid::types::storage_t& out_fame );


        private:
            std::string   m_port;
            conn_type_t   m_conn_type;
            os_sock_t     m_sock;
    };

}

}

#endif

