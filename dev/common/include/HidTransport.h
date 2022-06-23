#ifndef __hidtransport_h__
#define __hidtransport_h__

namespace hid {
namespace transport {

    using time_source_t = std::chrono::system_clock;
    using duration_ns_t = std::chrono::nanoseconds;
    using duration_us_t = std::chrono::microseconds;
    using duration_ms_t = std::chrono::milliseconds;
    using checkpoint_t  = std::chrono::time_point<time_source_t>;

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


}
}
#endif
