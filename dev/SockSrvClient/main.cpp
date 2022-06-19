#include <iostream>

#include <SockServerClient.h>
#include <StreamPrefix.h>


// const char* const           port        = "/tmp/serv.sock";
// hid::socket::conn_type_t    conn_type   = hid::socket::conn_type_t::CONN_TYPE_FILE;

const char* const           port = "4437";
hid::socket::conn_type_t    conn_type = hid::socket::conn_type_t::CONN_TYPE_SOCK;


int main() {

    hid::socket::sock_transaction_t tr;
    hid::socket::SocketServer srv;
    hid::socket::SocketClient cli;

    ::hid::types::storage_t out_fame;
    ::hid::types::storage_t inp_frame;


    srv.Start ( port, conn_type );
    std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );

    cli.Connect ( port, conn_type );
    std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );

    for ( int i = 0; i < 5; i++ ) {
        out_fame.resize ( out_fame.size () + 10 );
        cli.Transaction ( std::chrono::milliseconds ( 0 ), out_fame, inp_frame );
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );
    }

    cli.Close();
    std::this_thread::sleep_for ( std::chrono::milliseconds ( 5000 ) );

    srv.Stop ();
    std::this_thread::sleep_for ( std::chrono::milliseconds ( 5000 ) );

    for ( ; ; ) {
        std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
    }

    return 0;
}
