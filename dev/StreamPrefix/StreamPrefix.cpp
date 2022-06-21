#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

#include <StreamPrefix.h>
// #include <Serializer.h>


static void _test_no_expiration () {

    hid::types::storage_t       storage;
    hid::stream::params_t       params_in;
    hid::stream::params_t       params_out;
    hid::stream::duration_ms_t  expiration_ms;

    bool is_storage_valid;
    bool is_expiration_valid;

    params_out.command = hid::stream::cmd_t::STREAM_CMD_PING_REQUEST;
    params_out.code = 111;
    params_out.len  = 222;

    hid::stream::Prefix::SetParams ( params_out, storage );

    is_storage_valid = hid::stream::Prefix::Valid ( storage );

    hid::stream::Prefix::GetParams ( storage, params_in );
    hid::stream::Prefix::ExpirationTimeValid ( storage, is_expiration_valid );
    hid::stream::Prefix::ExpirationTimeGet ( storage, expiration_ms );

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
    hid::stream::duration_ms_t  expiration_ms;

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
    hid::stream::Prefix::ExpirationTimeGet ( storage, expiration_ms );

    assert ( is_storage_valid );
    assert ( is_expiration_valid );
    assert ( params_out.code == params_in.code );
    assert ( params_out.len == params_in.len );
    assert ( params_out.command == params_in.command );
}

static void _test_milliseconds () {

    hid::types::storage_t storage;
    int                   wait_time_ms = 5 * 1000;

    hid::stream::params_t       params = {};
    hid::stream::duration_ms_t  expiration_ms;

    hid::stream::Prefix::SetParams  ( params, storage );
    hid::stream::Prefix::SetTimeout ( std::chrono::milliseconds ( wait_time_ms ), storage );

    hid::stream::Prefix::ExpirationTimeGet ( storage, expiration_ms );

    assert ( wait_time_ms == expiration_ms.count() );
    return;
}

static void _test_seconds () {

    hid::types::storage_t storage;
    uint32_t              wait_time_s = 5;

    hid::stream::params_t       params = {};
    hid::stream::duration_ms_t  expiration_ms;

    hid::stream::Prefix::SetParams ( params, storage );
    hid::stream::Prefix::SetTimeout ( std::chrono::seconds ( wait_time_s ), storage );

    hid::stream::Prefix::ExpirationTimeGet ( storage, expiration_ms );

    assert ( (wait_time_s * 1000) ==  static_cast<uint32_t> (expiration_ms.count()) );
    return;
}

static void _test_bad () {

    hid::types::storage_t       storage;
    hid::stream::params_t       params_in;
    hid::stream::params_t       params_out;
    hid::stream::duration_ms_t  duration_ms;

    bool is_storage_valid;
    bool is_expiration_valid;

    params_out.command = hid::stream::cmd_t::STREAM_CMD_PING_REQUEST;
    params_out.code = 333;
    params_out.len = 444;

    params_in = params_out;
    duration_ms = hid::stream::duration_ms_t (555);

    hid::stream::Prefix::SetParams ( params_out, storage );
    hid::stream::Prefix::SetTimeout ( std::chrono::milliseconds ( 555 ), storage );

    storage[12]++;

    is_storage_valid    = hid::stream::Prefix::Valid ( storage );
    is_expiration_valid = hid::stream::Prefix::ExpirationTimeGet ( storage, duration_ms );

    hid::stream::Prefix::GetParams ( storage, params_in );

    assert ( ! is_storage_valid );
    assert ( ! is_expiration_valid );
    assert ( duration_ms.count() == 0);
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

    std::cout << "Done." << std::endl;
    return 0;
}
