#ifndef __SOCKSRVCLI_H__
#define __SOCKSRVCLI_H__

#include "SockOsTypes.h"
#include "SockTypes.h"


enum class conn_type_t {
    CONN_TYPE_UNKNOWN     = 0x00,
    CONN_TYPE_SOCK        = 0x10,
    CONN_TYPE_FILE        = 0x11
};

enum class frame_type_t {
    FRAME_TYPE_SYNC       = 0x20,
    FRAME_TYPE_REQUEST    = 0x21,
    FRAME_TYPE_RESPONSE   = 0x22,
    FRAME_TYPE_TIMEOUT    = 0x23
};

enum class sock_checkpoint_t {
    TIMEPOINT_RCV_HDR,
    TIMEPOINT_RCV_PAYLOAD,
    TIMEPOINT_SNT_HDR,
    TIMEPOINT_SNT_PAYLOAD,
};

constexpr int SOCK_SRV_MAX_ERRORS_CNT = 10;


class msg_header_t {

    public:
        msg_header_t ();
       ~msg_header_t ();
        
    public:
        void format ( frame_type_t type, uint32_t err_code, uint64_t timeout, uint32_t len );
        bool valid  ( sock_frame_t& frame );

    private:
        uint32_t calc_crc ( const void* const ptr );

    public:
        sock_frame_t  m_hdr;
};


class sock_transaction_t {

    public:
        sock_transaction_t ();
       ~sock_transaction_t ();
        sock_transaction_t ( const sock_transaction_t& ref );
        sock_transaction_t operator= ( const sock_transaction_t& ref ) = delete;

    public:
        void set_timeout    ( sock_duration_ms_t timeout_ms );
        void checkpoint_set ( sock_checkpoint_t point_type );
        void move_to_me     ( const sock_transaction_t& ref );
        bool is_expired     ();
        void close          ();

    public:
        sock_frame_t            in_data;
        sock_frame_t            out_data;
        sock_transaction_id_t   m_idx;
        sock_t                  m_sock;

    private:
        // Transaction life-cycle. 
        bool                    m_closed;
        sock_timepoint_t        m_start_time;
        sock_timepoint_t        m_rcv_hdr;
        sock_timepoint_t        m_rcv_payload;
        sock_timepoint_t        m_snt_hdr;
        sock_timepoint_t        m_snt_payload;
        sock_timepoint_t        m_commit_time;
        sock_timepoint_t        m_expiration_time;
};


// typedef std::map<sock_transaction_t, sock_transaction_id_t> sock_transaction_map_t;
typedef std::list<sock_transaction_t> sock_transaction_list_t;


class SocketServer {

    public:
        SocketServer();
       ~SocketServer();

    public:
        void  SetHandler ( ev_handler_t handler );

    public:
        bool  Start ( const char* const port, conn_type_t type );
        void  Stop  ( void );

    private:
        bool  Service ( sock_t socket );
        bool  Shell   ( sock_t socket );

    private:
        bool  ConnProcessNew ( sock_t server_socket );
        bool  ConnMoveToExpired ();
        bool  ConnProcessExpired ();
        bool  ConnRecvHdr ( sock_t client_socket );
        bool  ConnRecvPayload ( sock_t client_socket );
        bool  ConnSendHdr ( sock_t client_socket );
        bool  ConnSendPayload ( sock_t client_socket );

    private:
        bool  GetTimeout    ( bool expiration_valid, sock_timepoint_t exp_time, struct timeval& tv );
        bool  SendFrame     ( sock_t sock, const sock_frame_t& frame );
        bool  RecvFrame     ( sock_t sock, sock_frame_t& frame, sock_timepoint_t exp_time, bool exp_time_valid );

    private:
        ev_handler_t                m_handler;
        sock_clients_list_t         m_clients_list;
        sock_transaction_id_t       m_id;

    private:
        sock_transaction_list_t     m_pending_list;
        sock_transaction_list_t     m_rejected_list;
        std::mutex                  m_controller;

    private:
        int                         m_err_cnt;
        sock_t                      m_server_sock;
        bool                        m_stop;
        sock_thread_t               m_server;
        sock_conn_list_t            m_clients;
};



class SocketClient {

    public:
        SocketClient();
       ~SocketClient();

    public:
        bool Connect ( const char* const port, conn_type_t type );
        bool Transaction ( sock_duration_ms_t duration, const sock_frame_t& out_fame, sock_frame_t& in_frame, frame_type_t& status );

    private:
        sock_t                      m_client_sock;
        expiration_t                expiration_time;
        std::string                 m_port;
        conn_type_t                 m_type;
};


#endif

