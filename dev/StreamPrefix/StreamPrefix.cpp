#include <iostream>
#include <StreamPrefix.h>
#include <Serializer.h>

#if 0

void test_steady () {

    uint64_t val = 0;

    {   // steady_clock to UINT64
        std::chrono::time_point<std::chrono::steady_clock> res1 = std::chrono::steady_clock::now();
        val = res1.time_since_epoch ().count ();
    }

    {   // UINT64 to steady_clock
        using steady_time_point_t = std::chrono::steady_clock::time_point;
        steady_time_point_t exp_time1{ std::chrono::duration_cast<steady_time_point_t::duration> (std::chrono::nanoseconds(val)) };
    }


    {   // UINT64 to steady_clock
        using steady_time_point_t = std::chrono::steady_clock::time_point;
        using suration_ns_t = std::chrono::nanoseconds;

        steady_time_point_t exp_time1{ std::chrono::duration_cast<steady_time_point_t::duration> (suration_ns_t(val)) };
    }

}

void test_system () {

    uint64_t val = 0;

    {   // system_clock to UINT64
        std::chrono::time_point<std::chrono::system_clock> res1 = std::chrono::system_clock::now();
        val = res1.time_since_epoch ().count ();
    }

    {   // UINT64 to system_clock
        using system_time_point_t = std::chrono::steady_clock::time_point;
        system_time_point_t exp_time1{ std::chrono::duration_cast<system_time_point_t::duration> (std::chrono::nanoseconds(val)) };
        system_time_point_t exp_time2{ std::chrono::duration_cast<system_time_point_t::duration> (std::chrono::milliseconds(val / 1000 / 1000)) };
    }


    {   using duration_millisec_t = std::chrono::duration<int, std::milli>;
        using time_point_t = std::chrono::time_point<std::chrono::system_clock>;

        time_point_t start = std::chrono::system_clock::now();
        duration_millisec_t diff ( 500 );
        time_point_t end = start + diff;

        // for ( int i = 0; i < 1000; i++ ) {
        //     start += duration_millisec_t(120);
        //     if ( start > end ) {
        //         break;
        //     }
        // }

    }

}

                                
int main () {

    hid::stream_params_t    params_out;
    hid::stream_params_t    params_in;
    hid::serializer_bin_t   storage;
    
    hid::StreamPrefix       prefix;

    test_steady ();
    test_system ();


    params_out.command   = hid::StreamCmd::STREAM_CMD_REQUEST;
    params_out.code      = 102;
    params_out.len       = 66;

    prefix.Format (params_out, storage);
    prefix.IsValid (storage);
    prefix.Load (storage, params_in);


    return 0;
}

#endif


int main () {

    hid::serializer_bin_t   storage;

    hid::stream_params_t    params_out;
    hid::StreamPrefix       prefix_out;

    hid::stream_params_t    params_in;
    hid::StreamPrefix       prefix_in;

    bool                    is_exp_valid;
    bool                    is_expired;
    struct timeval          tv;

    params_out.command = hid::StreamCmd::STREAM_CMD_REQUEST;
    params_out.code = 100;
    params_out.len = 578;

    prefix_out.Format (params_out, storage);
    prefix_out.SetTimeout (60 * 1000, storage);

    prefix_in.Valid (storage);
    prefix_in.Load (storage, params_in);
    prefix_in.ExpirationTimeValid (storage, is_exp_valid);

    while( true ) {
        prefix_in.ExpirationTimeGet (storage, is_expired, tv);
        if ( is_expired ) {
            break;
        }
    }

    return 0;
}
