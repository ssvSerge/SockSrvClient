#ifndef __SOCKSERVERCLIENT_H__
#define __SOCKSERVERCLIENT_H__

#include <chrono>
#include <list>
#include <future>
#include <thread>
#include <atomic>

#include <HidTransportInt.h>

namespace hid {

namespace transport {


    class SocketServer : public TransportServer {

        public:
            SocketServer ();
            virtual ~SocketServer ();

        public:
            virtual void  SetHandler ( ev_handler_t handler );
            virtual bool  Start ( const char* const port, conn_type_t conn_type );
            virtual void  Stop  ( void );

        private:
            void  Service          ( void );
            bool  Shell            ( os_sock_t socket );
            void  ShellCmdStart    ( os_sock_t socket, conn_state_t& state, transaction_t& tr );
            void  ShellReadPrefix  ( os_sock_t socket, conn_state_t& state, transaction_t& tr );
            void  ShellReadPayload ( os_sock_t socket, conn_state_t& state, transaction_t& tr );
            void  ShellCmdExec     ( os_sock_t socket, conn_state_t& state, transaction_t& tr );
            void  ShellSendPrefix  ( os_sock_t socket, conn_state_t& state, transaction_t& tr );
            void  ShellSendPayload ( os_sock_t socket, conn_state_t& state, transaction_t& tr );
            void  ShellClose       ( os_sock_t socket, const conn_state_t& state );
            void  LogTransaction   ( const transaction_t& tr, const conn_state_t state );

        private:
            void  StartClient      ( conn_state_t& state, os_sock_t client_sock );

        private:
            std::mutex          m_controller;
            clients_list_t      m_clients;
    };

    class SocketClient : public TransportClient {

        public:
            SocketClient ();
            virtual ~SocketClient ();

        public:
            virtual bool Connect ( const char* const port, conn_type_t conn_type );
            virtual void Close ();
            virtual bool Transaction ( std::chrono::milliseconds delayMs, const hid::types::storage_t& out_frame, hid::types::storage_t& in_frame, uint32_t& in_code );
            virtual bool Transaction ( const hid::types::storage_t& out_frame, hid::types::storage_t& in_frame, uint32_t& in_code );

        private:
            void connect        ( conn_state_t& state );
            void SendPrefix     ( conn_state_t& state, std::chrono::milliseconds delay_ms, transaction_t& tr, size_t out_fame_len );
            void SendPayload    ( conn_state_t& state, transaction_t& tr, const hid::types::storage_t& out_fame );
            void RecvHeader     ( conn_state_t& state, transaction_t& tr );
            void RecvPayload    ( conn_state_t& state, transaction_t& tr, hid::types::storage_t& in_frame );
            void LogTransaction ( const transaction_t& tr, const conn_state_t conn_state );
            bool TransactionInt ( conn_state_t& state, std::chrono::milliseconds delay_ms, transaction_t& tr, const hid::types::storage_t& out_fame );


        private:
            os_sock_t     m_sock;
    };

}

}

#endif

