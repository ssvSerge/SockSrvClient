#include <iostream>
#include <StreamPrefix.h>
#include <Serializer.h>

int main () {

    hid::types::storage_t           storage;

    hid::stream::stream_params_t    params_out;
    hid::stream::StreamPrefix       prefix_out;

    hid::stream::stream_params_t    params_in;
    hid::stream::StreamPrefix       prefix_in;

    bool                            is_exp_valid;
    bool                            is_expired;
    struct timeval                  tv;

    params_out.command  = hid::stream::StreamCmd::STREAM_CMD_REQUEST;
    params_out.code     = 100;
    params_out.len      = 578;

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
