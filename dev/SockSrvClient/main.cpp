#include <iostream>

#include <SockServerClient.h>

const char* const           port        = "/tmp/serv.sock";
hid::socket::conn_type_t    conn_type   = hid::socket::conn_type_t::CONN_TYPE_FILE;


int main() {

    hid::socket::sock_transaction_t tr;
    hid::socket::SocketServer srv;
    hid::socket::SocketClient cli;

    ::hid::types::storage_t     out_fame;
    ::hid::types::storage_t     in_frame;
    out_fame.resize ( 16 );

    srv.Start ( port, conn_type );
    std::this_thread::sleep_for ( std::chrono::milliseconds ( 5000 ) );

    cli.Connect ( port, conn_type );
    cli.Transaction ( std::chrono::milliseconds(0), out_fame, in_frame );

    std::cout << "Hello World!\n";
}
