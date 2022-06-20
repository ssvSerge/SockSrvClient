#include <iostream>

#include <SockServerClient.h>
#include <StreamPrefix.h>


// const char* const           port        = "/tmp/serv.sock";
// hid::socket::conn_type_t    conn_type   = hid::socket::conn_type_t::CONN_TYPE_FILE;

const char* const           port = "4437";
hid::socket::conn_type_t    conn_type = hid::socket::conn_type_t::CONN_TYPE_SOCK;


#if 0
int main() {

    hid::socket::sock_transaction_t tr;
    hid::socket::SocketServer srv;
    hid::socket::SocketClient cli;

    ::hid::types::storage_t out_fame;
    ::hid::types::storage_t inp_frame;

    for ( int i = 0; i <5; i++ ) {
        srv.Start ( port, conn_type );
        for( int j = 0; j < 5; j++ ) {

            if ( i != 3 ) {
                cli.Connect ( port, conn_type );
            }

            for ( int k = 0; k < 5; k++ ) {
                out_fame.resize ( out_fame.size () + 100 );
                cli.Transaction ( std::chrono::milliseconds ( 0 ), out_fame, inp_frame );
            }

            if ( i == 2 ) {
                break;
            }

            cli.Close ();
        }
        srv.Stop ();
    }

    cli.Close();
    srv.Stop ();

    // std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
    return 0;
}

#endif

int main () {

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

    // std::this_thread::sleep_for ( std::chrono::milliseconds ( 1000 ) );
    return 0;
}



