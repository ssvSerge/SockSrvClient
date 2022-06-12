#include <stdint.h>

#include <HidTypes.h>
#include <HidOsTypes.h>

#include <chrono>
#include <ctime>

namespace hid {

    #define PREFIX_MAGIC    (0x37464564)

    enum class StreamCmd : uint32_t {
        STREAM_CMD_NONE       =   0,
        STREAM_CMD_REQUEST    = 100,
        STREAM_CMD_RESPONSE   = 101,
        STREAM_CMD_PING       = 102
    };

    class stream_params_t {
        public:
            StreamCmd       command;
            uint32_t        code;
            uint32_t        len;
    };

    class StreamPrefix {

        using time_source_t      = std::chrono::steady_clock;
        using time_point_t       = std::chrono::time_point<time_source_t>;

        using time_duration_ns_t = std::chrono::nanoseconds;
        using time_duration_ms_t = std::chrono::milliseconds;

        public:

            bool Format (const stream_params_t& params, hid::serializer_bin_t& storage) const {

                bool ret_val = false;

                try {

                    const size_t len = PrefixSize ();
                    storage.resize ( len );

                    uint32_t* ptr = reinterpret_cast<uint32_t*> (storage.data ());

                    memset ( storage.data (), 0xCC, len );

                    ptr [0] = PREFIX_MAGIC;
                    ptr [1] = static_cast<uint32_t> (params.command);
                    ptr [2] = params.code;
                    ptr [3] = 0; // Placement for TIMEOUT.
                    ptr [4] = 0; // Shall be configured separately.
                    ptr [5] = params.len;
                    ptr [6] = 0xFFFFFFFF - ptr [0] - ptr [1] - ptr [2] - ptr [3] - ptr [4] - ptr [5];

                    ret_val = true;

                } catch( ... ) {
                    // LOG Exception. No Memory could be thrown.
                }

                return ret_val;
            }

            bool SetTimeout ( uint32_t timeout_ms, hid::serializer_bin_t& storage ) const {

                bool ret_val = false;

                if ( storage.size () == PrefixSize() ) {

                    if ( timeout_ms > 0 ) {

                        const time_duration_ms_t delay_ms (timeout_ms);

                        const time_point_t  my_time_ns   = time_source_t::now ();
                        const time_point_t  exp_time_ns  = (my_time_ns + delay_ms);
                        const uint64_t      abs_time_ns  = exp_time_ns.time_since_epoch().count();

                        uint32_t* const    ptr            = reinterpret_cast<uint32_t*> ( storage.data () );
                        uint64_t* const    exp_ptr        = reinterpret_cast<uint64_t*> ( storage.data () + TsOffset() );

                        *exp_ptr = abs_time_ns;
                        ptr [6] = 0xFFFFFFFF - ptr [0] - ptr [1] - ptr [2] - ptr [3] - ptr [4] - ptr [5];
                    }


                    ret_val = true;
                }

                return true;
            }

            bool Load ( const hid::serializer_bin_t& storage, stream_params_t& params ) const {

                bool ret_val = false;

                if ( Valid ( storage ) ) {

                    const uint32_t* const ptr = reinterpret_cast<const uint32_t*> ( storage.data() );

                    params.command   = static_cast<StreamCmd> (ptr [1]);
                    params.code      = ptr [2];
                    params.len       = ptr [5];

                    ret_val = true;
                }

                return ret_val;
            }

            bool ExpirationTimeValid ( const hid::serializer_bin_t& storage, bool& is_valid ) const  {

                bool ret_val = false;

                is_valid = false;

                if ( storage.size () == PrefixSize () ) {

                    const uint64_t* const expiration_time_ns = reinterpret_cast<const uint64_t*> ( storage.data () + TsOffset() );

                    if ( expiration_time_ns [0] != 0 ) {
                        is_valid = true;
                    }

                    ret_val = true;
                }

                return ret_val;
            }

            bool ExpirationTimeGet ( const hid::serializer_bin_t& storage, bool& is_expired, struct timeval& tv ) const {

                bool ret_val = false;

                bool is_expiration_defined = false;

                is_expired = true; // Assumee we're expired.
                tv.tv_sec  = 0;    // Seconds
                tv.tv_usec = 0;    // microseconds

                if ( ExpirationTimeValid(storage, is_expiration_defined) ) {

                    if ( is_expiration_defined ) {

                        const uint64_t* const src_ptr = reinterpret_cast<const uint64_t*> ( storage.data () + TsOffset() );

                        if ( src_ptr[0] != 0 ) {

                            const time_duration_ns_t exp_time_ns (src_ptr[0]);
                            const time_duration_ms_t deviation_time_ms ( 1 );
                            const time_point_t tp_now  = time_source_t::now ();
                            const time_point_t tp_exp { exp_time_ns - deviation_time_ms };

                            auto test = (tp_exp - tp_now );
                                  
                            if ( tp_now < tp_exp ) {

                                time_duration_ns_t wait_time_ns = (tp_exp - tp_now);
                                uint64_t microseconds_cnt = wait_time_ns.count () / 1000;

                                tv.tv_sec  = static_cast<long> (microseconds_cnt / 1000000);  // Seconds
                                tv.tv_usec = static_cast<long> (microseconds_cnt % 1000000);  // microseconds

                                is_expired = false;
                            }

                        }

                    }

                    ret_val = true;

                }

                return ret_val;

            }

            bool Valid ( const hid::serializer_bin_t& storage ) const {
                
                bool ret_val = false;

                size_t len = PrefixSize();
                
                if ( storage.size() == len ) {
                    const uint32_t* const ptr = reinterpret_cast<const uint32_t*> ( storage.data() );

                    uint32_t crc = 0;

                    crc = 0xFFFFFFFF - ptr[0] - ptr[1] - ptr[2] - ptr[3] - ptr[4] - ptr[5];

                    if ( crc == ptr [6] ) {
                        ret_val = true;
                    }
                }

                return ret_val;                
            }

        private:

            constexpr size_t PrefixSize() const {
                // +  0 uint32_t     magic
                // +  4 uint32_t     command
                // +  8 uint32_t     code
                // + 12 uint64_t     timestamp
                // + 20 uint32_t     payload_len
                // + 24 uint32_t     crc
                // + 28
                return 28;
            }

            constexpr int TsOffset() const {
                return 12;
            }

    };

}


