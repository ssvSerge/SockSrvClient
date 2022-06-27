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

        private:
            virtual bool  StartMe  ( const char* const port, conn_type_t conn_type ) override;
            virtual void  StopMe   ( void ) override;

        private:
            void  Service          ( void );
            bool  Shell            ( os_sock_t sock );
            void  StartClient      ( conn_state_t& state, os_sock_t client_sock );
            void  ShellClose       ( os_sock_t socket, const conn_state_t& state );

        private:
            void  ShellCmdStart    ( os_sock_t sock, conn_state_t& state, transaction_t& tr );
            void  ShellReadPrefix  ( os_sock_t sock, conn_state_t& state, transaction_t& tr );
            void  ShellReadPayload ( os_sock_t sock, conn_state_t& state, transaction_t& tr );
            void  ShellCmdExec     ( os_sock_t sock, conn_state_t& state, transaction_t& tr );
            void  ShellSendPrefix  ( os_sock_t sock, conn_state_t& state, transaction_t& tr );
            void  ShellSendPayload ( os_sock_t sock, conn_state_t& state, transaction_t& tr );
            void  LogTransaction   ( const transaction_t& tr, const conn_state_t state );

        private:
            clients_list_t  m_clients;
    };

    class SocketClient : public TransportClient {

        public:
            SocketClient ();
            virtual ~SocketClient ();

        public:
            virtual bool Connect ( const char* const port, conn_type_t conn_type ) override;
            virtual void Close () override;
            virtual bool Transaction ( duration_ms_t delay_ms, const hid::types::storage_t& out_frame, hid::types::storage_t& in_frame, uint32_t& in_code ) override;
            virtual bool Sync ( duration_ms_t delay_ms, uint32_t rand );

        private:
            void connect        ( conn_state_t& state );
            void SendPrefix     ( conn_state_t& state, duration_ms_t delay_ms, transaction_t& tr, size_t out_fame_len );
            void SendPayload    ( conn_state_t& state, transaction_t& tr, const hid::types::storage_t& out_fame );
            void RecvHeader     ( conn_state_t& state, transaction_t& tr );
            void RecvPayload    ( conn_state_t& state, transaction_t& tr, hid::types::storage_t& in_frame );
            void LogTransaction ( const transaction_t& tr, const conn_state_t conn_state );
            bool TransactionInt ( conn_state_t& state, duration_ms_t delay_ms, transaction_t& tr, const hid::types::storage_t& out_fame );

        private:
            os_sock_t       m_sock;
    };

}

}

#endif

