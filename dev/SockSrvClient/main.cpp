#include <iostream>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <cassert>

#include <SockServerClient.h>
#include <StreamPrefix.h>

using namespace std::chrono_literals;

namespace hid {
namespace transport {
extern uint32_t SOCK_TEXT_TX_DELAY;
extern std::atomic<int> g_sockets_cnt;
}
}

auto handler_delay = 500ms;

hid::types::storage_t out_data_local;
static uint32_t cmd_id_local = 340;
constexpr int DUMMY_SIZE = 512;
constexpr int DUMMY_RET_CODE = 432;

void cmd_handler ( const hid::types::storage_t& in_data, hid::types::storage_t& out_data, uint32_t& error_code ) {

    UNUSED ( in_data );

    std::this_thread::sleep_for ( handler_delay );

    size_t new_size = 100;
    new_size += static_cast<int> (std::rand () % 200);

    out_data_local.resize( new_size );

    for ( size_t i=0; i< new_size; i++ ) {
        out_data_local[i] = 'a' + (std::rand () % 20);
    }

    cmd_id_local++;
    error_code = cmd_id_local;
    out_data = out_data_local;
}


static void test_sync_01 () {

    const char* const               port = "4400";
    hid::transport::conn_type_t     conn_type = hid::transport::conn_type_t::CONN_TYPE_FILE;
    hid::transport::SocketServer    srv;
    hid::transport::SocketClient    cli;
    uint32_t rnd = 0;

    srv.Start ( port, conn_type );
    cli.Connect ( port, conn_type );

    for ( int i = 0; i < 10; i++ ) {
        rnd = rand();
        cli.Sync( 100ms, rnd );
    }
}

static void test_timeout_02 () {

    const char* const               port = "4400";
    hid::transport::conn_type_t     conn_type = hid::transport::conn_type_t::CONN_TYPE_SOCK;
    uint32_t                        ret_code;
    bool                            ret_val;
    hid::transport::SocketServer    srv;
    hid::transport::SocketClient    cli;
    hid::types::storage_t           out_fame;
    hid::types::storage_t           inp_frame;

    srv.Start ( port, conn_type );
    srv.SetHandler ( cmd_handler );
    cli.Connect ( port, conn_type );

    handler_delay = 400ms;
    for ( int i = 1; i < 20; i++ ) {

        inp_frame.clear();
        inp_frame.resize( DUMMY_SIZE );
        ret_code = DUMMY_RET_CODE;

        ret_val = cli.Transaction ( std::chrono::milliseconds ( i * 100ms ), out_fame, inp_frame, ret_code );
        std::cout << "cli.Transaction (" << i << ") Res: " << ret_val << "; Code: " << ret_code << std::endl;
        if ( ret_val ) {
            assert ( inp_frame == out_data_local );
            assert ( ret_code  == cmd_id_local );
        } else {
            assert ( inp_frame.size() == 0 );
            assert ( ret_code == 0 );
        }
    }

    cli.Close ();
    srv.Stop ();
}

static void test_tx_delay_03 () {

    const char* const               port = "4401.sock";
    hid::transport::conn_type_t     conn_type = hid::transport::conn_type_t::CONN_TYPE_FILE;

    hid::transport::transaction_t tr;
    hid::transport::SocketServer srv;
    hid::transport::SocketClient cli;

    hid::types::storage_t out_fame;
    hid::types::storage_t inp_frame;

    uint32_t ret_code;
    bool     ret_val;

    srv.Start   ( port, conn_type );
    cli.Connect ( port, conn_type );

    srv.SetHandler ( cmd_handler );
    handler_delay = 10ms;

    hid::transport::SOCK_TEXT_TX_DELAY = 1;
    for( int i = 1; i < 10; i++ ) {

        ret_val = cli.Transaction ( 5s, out_fame, inp_frame, ret_code );
        if ( ret_val ) {
            assert ( inp_frame == out_data_local );
            assert ( ret_code == cmd_id_local );
        } else {
            assert ( inp_frame.size () == 0 );
            assert ( ret_code == 0 );
        }
        std::cout << "Transaction (" << i << ") " << std::endl;
    }

    cli.Close ();
    srv.Stop ();
}

int main () {

    time_t dummy = std::time ( nullptr );
    std::srand ( static_cast<int>(dummy) );

    test_sync_01     ();
    test_timeout_02  ();
    test_tx_delay_03 ();

    std::cout << "Done" << std::endl;
    return 0;
}
