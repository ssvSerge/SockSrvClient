#ifndef __hidtransportint_h__
#define __hidtransportint_h__

#include <chrono>
#include <future>

#include <HidTypes.h>
#include <HidOsTypes.h>

namespace hid {
namespace transport {

    using time_source_t  =  std::chrono::system_clock;
    using duration_ns_t  =  std::chrono::nanoseconds;
    using duration_us_t  =  std::chrono::microseconds;
    using duration_ms_t  =  std::chrono::milliseconds;
    using checkpoint_t   =  std::chrono::time_point<time_source_t>;

    constexpr auto COMM_TIMEOUT = std::chrono::milliseconds ( 5 * 1000 );

    enum class conn_type_t {
        CONN_TYPE_UNKNOW        = 0,
        CONN_TYPE_SOCK          = 10,
        CONN_TYPE_FILE          = 11,
        CONN_TYPE_USB           = 12
    };

    enum class checkpoint_id_t {
        CHECKPOINT_UNKNOWN      =  0,
        CHECKPOINT_START        = 10,
        CHECKPOINT_RX_HDR       = 11,
        CHECKPOINT_RX_PAYLOAD   = 12,
        CHECKPOINT_EXEC         = 13,
        CHECKPOINT_TX_HDR       = 14,
        CHECKPOINT_TX_PAYLOAD   = 15
    };

    enum class conn_state_t {
        CONN_STATE_UNKNOWN      =   0,
        CONN_OK                 = 100,
        CONN_RX_DONE            = 200,
        CONN_TX_DONE            = 300,
        CONN_ERR_GENERAL        = 400,
        CONN_ERR_CLOSED         = 401,
        CONN_ERR_TIMEOUT        = 402,
        CONN_ERR_OPEN           = 403,
        CONN_ERR_BIND           = 404,
        CONN_ERR_LISTEN         = 405,
        CONN_ERR_SELECT         = 406,
        CONN_ERR_ACCEPT         = 407,
        CONN_ERR_CONNECT        = 408,
        CONN_ERR_RX             = 409,
        CONN_ERR_TX             = 410,
        CONN_ERR_EXEC           = 411,
        CONN_ERR_SYNC           = 500
    };

    typedef void (*ev_handler_t) (const hid::types::storage_t& in_data, hid::types::storage_t& out_data, uint32_t& error_code);

    typedef std::future<bool>           sock_thread_t;
    typedef std::list<sock_thread_t>    clients_list_t;

    class transaction_t {

        public:
            transaction_t ();
            transaction_t ( const transaction_t& ref ) = delete;
            transaction_t  operator= ( const transaction_t& ref ) = delete;

        public:
            void start ( duration_ms_t expiration_default_ms );
            void expiration_set ( duration_ms_t expiration_ms );
            void checkpoint_set ( checkpoint_id_t point_type );
            void reset ( void );

        public:
            uint32_t                inp_cmd;
            uint32_t                inp_code;
            uint32_t                out_cmd;
            uint32_t                out_code;
            hid::types::storage_t   inp_hdr;
            hid::types::storage_t   inp_pay;
            hid::types::storage_t   out_hdr;
            hid::types::storage_t   out_pay;

        public:
            checkpoint_t            tv_start;
            checkpoint_t            tv_rcv_hdr;
            checkpoint_t            tv_rcv_pay;
            checkpoint_t            tv_exec;
            checkpoint_t            tv_snt_hdr;
            checkpoint_t            tv_snt_pay;
            checkpoint_t            tv_expiration;
    };

    class TransportServer {

        public:
            void  SetHandler ( ev_handler_t handler );
            bool  Start ( const char* const port, conn_type_t conn_type );
            void  Stop  ( void );

        private:
            virtual bool  StartMe ( const char* const port, conn_type_t conn_type ) = 0;
            virtual void  StopMe ( void ) = 0;

        protected:
            std::atomic<bool>   m_stop;                 // Request to shutdown.
            std::atomic<bool>   m_instance_active;      // Only one active instance allowed.
            std::mutex          m_access_controller;    // Prevent concurrent access from threads.
            std::string         m_port;                 // Configuration sting. Port/File/etc.
            conn_type_t         m_conn_type;            // 
            ev_handler_t        m_ev_handler;           // 
            std::thread         m_server_thread;        // 

    };

    class TransportClient {

        protected:
            virtual bool Connect ( const char* const port, conn_type_t conn_type ) = 0;
            virtual void Close () = 0;
            virtual bool Transaction ( duration_ms_t delayMs, const hid::types::storage_t& out_frame, hid::types::storage_t& in_frame, uint32_t& in_code ) = 0;

        protected:
            std::string   m_port;
            conn_type_t   m_conn_type;
    };

}
}

#endif
