#include <iostream>
#include <chrono>

#include <SockServerClient.h>
#include <StreamPrefix.h>

using namespace std::chrono_literals;

namespace hid {
namespace socket {

extern uint32_t SOCK_TEXT_TX_DELAY;

}
}

void cmd_handler ( const hid::types::storage_t& in_data, hid::types::storage_t& out_data, uint32_t& error_code ) {

    UNUSED ( in_data );
    UNUSED ( out_data );
    UNUSED ( error_code );

    std::this_thread::sleep_for ( 6000ms );
}


static void test_sock_01 () {

    const char* const           port = "4400";
    hid::socket::conn_type_t    conn_type = hid::socket::conn_type_t::CONN_TYPE_SOCK;

    hid::socket::sock_transaction_t tr;
    hid::socket::SocketServer srv;
    hid::socket::SocketClient cli;

    ::hid::types::storage_t out_fame;
    ::hid::types::storage_t inp_frame;

    srv.Start ( port, conn_type );
    cli.Connect ( port, conn_type );

    for ( int i = 0; i < 30; i++ ) {

        if ( i == 10 ) {
            srv.Stop ();
        }

        if ( i == 20 ) {
            srv.Start ( port, conn_type );
        }

        cli.Transaction ( std::chrono::milliseconds ( 90 * 1000 ), out_fame, inp_frame );
    }

    cli.Close ();
    srv.Stop ();
}

static void test_file_02 () {

    const char* const           port = "4401.sock";
    hid::socket::conn_type_t    conn_type = hid::socket::conn_type_t::CONN_TYPE_FILE;

    hid::socket::sock_transaction_t tr;
    hid::socket::SocketServer srv;
    hid::socket::SocketClient cli;

    ::hid::types::storage_t out_fame;
    ::hid::types::storage_t inp_frame;

    srv.Start ( port, conn_type );
    cli.Connect ( port, conn_type );

    for ( int i = 0; i < 30; i++ ) {

        if ( i == 10 ) {
            srv.Stop ();
        }

        if ( i == 20 ) {
            srv.Start ( port, conn_type );
        }

        cli.Transaction ( std::chrono::milliseconds ( 0 ), out_fame, inp_frame );
    }

    cli.Close ();
    srv.Stop ();
}

static void test_sock_03 () {

    const char* const           port = "4403";
    hid::socket::conn_type_t    conn_type = hid::socket::conn_type_t::CONN_TYPE_SOCK;

    hid::socket::sock_transaction_t tr;
    hid::socket::SocketServer srv;
    hid::socket::SocketClient cli;

    hid::types::storage_t out_fame;
    hid::types::storage_t inp_frame;
    bool ret_val;


    hid::socket::SOCK_TEXT_TX_DELAY = 3;

    srv.Start ( port, conn_type );
    srv.SetHandler ( cmd_handler );
    cli.Connect ( port, conn_type );
    out_fame.resize (50);
    
    ret_val = cli.Transaction ( std::chrono::milliseconds ( 5 * 1000 ), out_fame, inp_frame );
    std::cout << "transaction status: " << ret_val << std::endl;

    cli.Close ();
    srv.Stop ();
}


int main () {

    // test_sock_01 ();
    // test_file_02 ();
    test_sock_03 ();

    return 0;
}
