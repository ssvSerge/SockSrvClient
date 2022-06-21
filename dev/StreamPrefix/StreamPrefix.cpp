#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

#include <StreamPrefix.h>
#include <Serializer.h>


static void _test_no_expiration () {

    hid::types::storage_t       storage;
    hid::stream::params_t       params_in;
    hid::stream::params_t       params_out;
    hid::stream::time_point_t   tp_expiration;

    bool is_storage_valid;
    bool is_expiration_valid;

    params_out.command = hid::stream::cmd_t::STREAM_CMD_PING_REQUEST;
    params_out.code = 111;
    params_out.len  = 222;

    hid::stream::Prefix::SetParams ( params_out, storage );

    is_storage_valid = hid::stream::Prefix::Valid ( storage );

    hid::stream::Prefix::GetParams ( storage, params_in );
    hid::stream::Prefix::ExpirationTimeValid ( storage, is_expiration_valid );
    hid::stream::Prefix::ExpirationTimeGet ( storage, tp_expiration );

    assert ( is_storage_valid );
    assert ( ! is_expiration_valid );
    assert ( params_out.code == params_in.code );
    assert ( params_out.len == params_in.len );
    assert ( params_out.command == params_in.command );
}

static void _test_no_with_expiration () {

    hid::types::storage_t       storage;
    hid::stream::params_t       params_in;
    hid::stream::params_t       params_out;
    hid::stream::time_point_t   tp_expiration;

    bool is_storage_valid;
    bool is_expiration_valid;
    int  exp_time_out  = 10 * 1000;

    params_out.command = hid::stream::cmd_t::STREAM_CMD_PING_REQUEST;
    params_out.code = 333;
    params_out.len  = 444;

    hid::stream::Prefix::SetParams ( params_out, storage );
    hid::stream::Prefix::SetTimeout ( std::chrono::milliseconds(exp_time_out), storage );

    is_storage_valid = hid::stream::Prefix::Valid ( storage );

    hid::stream::Prefix::GetParams ( storage, params_in );
    hid::stream::Prefix::ExpirationTimeValid ( storage, is_expiration_valid );
    hid::stream::Prefix::ExpirationTimeGet ( storage, tp_expiration );

    assert ( is_storage_valid );
    assert ( is_expiration_valid );
    assert ( params_out.code == params_in.code );
    assert ( params_out.len == params_in.len );
    assert ( params_out.command == params_in.command );
}

static void _test_milliseconds () {

    hid::types::storage_t storage;
    bool                  is_expired;
    struct timeval        tv;
    int                   wait_time_ms = 5 * 1000;

    hid::stream::params_t       params = {};
    hid::stream::time_point_t   exp_time;

    hid::stream::Prefix::SetParams  ( params, storage );
    hid::stream::Prefix::SetTimeout ( std::chrono::milliseconds ( wait_time_ms ), storage );

    hid::stream::Prefix::ExpirationTimeGet ( storage, exp_time );

    hid::stream::time_point_t start_time = hid::stream::time_source_t::now ();
    while( true ) {
        hid::stream::Prefix::WaitTimeGet ( exp_time, tv, is_expired );
        if ( is_expired ) {
            break;
        }
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 10 ) );
    }
    hid::stream::time_point_t end_time = hid::stream::time_source_t::now();

    std::cout << "Wait time: " << std::chrono::duration_cast<std::chrono::milliseconds> (end_time - start_time).count() << "ms; ";
    std::cout << "Expected time: " << wait_time_ms << "ms; " << std::endl;
    return;
}

static void _test_seconds () {

    hid::types::storage_t storage;
    bool                  is_expired;
    struct timeval        tv;
    int                   wait_time_s = 5;

    hid::stream::params_t       params = {};
    hid::stream::time_point_t   exp_time;

    hid::stream::Prefix::SetParams ( params, storage );
    hid::stream::Prefix::SetTimeout ( std::chrono::seconds ( wait_time_s ), storage );

    hid::stream::Prefix::ExpirationTimeGet ( storage, exp_time );

    hid::stream::time_point_t start_time = hid::stream::time_source_t::now ();
    while( true ) {
        hid::stream::Prefix::WaitTimeGet ( exp_time, tv, is_expired );
        if( is_expired ) {
            break;
        }
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 10 ) );
    }
    hid::stream::time_point_t end_time = hid::stream::time_source_t::now ();

    std::cout << "Wait time: " << std::chrono::duration_cast<std::chrono::milliseconds> (end_time - start_time).count () << "ms; ";
    std::cout << "Expected time: " << wait_time_s * 1000 << "ms; " << std::endl;
    return;
}

static void _test_bad () {

    hid::types::storage_t       storage;
    hid::stream::params_t       params_in;
    hid::stream::params_t       params_out;
    hid::stream::time_point_t   tp_expiration;

    bool is_storage_valid;
    bool is_expiration_valid;

    params_out.command = hid::stream::cmd_t::STREAM_CMD_PING_REQUEST;
    params_out.code = 333;
    params_out.len = 444;

    params_in = params_out;

    hid::stream::Prefix::SetParams ( params_out, storage );
    hid::stream::Prefix::SetTimeout ( std::chrono::milliseconds ( 555 ), storage );

    storage[12]++;

    is_storage_valid    = hid::stream::Prefix::Valid ( storage );
    is_expiration_valid = hid::stream::Prefix::ExpirationTimeGet ( storage, tp_expiration );

    hid::stream::Prefix::GetParams ( storage, params_in );

    assert ( ! is_storage_valid );
    assert ( ! is_expiration_valid );
    assert ( params_in.code == 0 );
    assert ( params_in.len == 0);
    assert ( params_in.command == hid::stream::cmd_t::STREAM_CMD_NONE );
}

int main () {

    _test_no_expiration();
    _test_no_with_expiration();
    _test_bad ();
    _test_milliseconds ();
    _test_seconds ();

    return 0;
}
