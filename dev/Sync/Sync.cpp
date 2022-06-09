#include <thread>
#include <chrono>

#include "SockServerClient.h"
#include "Serializer.h"

// #define CONN_PORT     ("c://temp//sock_conn")
// #define CONN_TYPE     (conn_type_t::CONN_TYPE_FILE)

#define CONN_PORT     ("4567")
#define CONN_TYPE     (conn_type_t::CONN_TYPE_SOCK)

using namespace std::chrono_literals;


int main() {

    int8_t      v01 = 0x55;
    uint8_t     v02 = 0x56;
    int16_t     v03 = 0x595A;
    uint16_t    v04 = 0x5758;
    int32_t     v05 = 0x5F606162;
    uint32_t    v06 = 0x5B5C5D5E;
    int64_t     v07 = 0x6B6C6D6E6F707172U;
    uint64_t    v08 = 0x636465666768696AU;
    float       v09 = 0.1;
    double      v10 = 0.33;

    hid::Serializer obj;
    hid::serializer_storage_t storage;


    obj.Reset ();
    obj.StoreArr (8,   storage);

    obj.StoreFix (v01, storage);
    obj.StoreFix (v02, storage);
    obj.StoreFix (v03, storage);
    obj.StoreFix (v04, storage);

    obj.StoreFix (v05, storage);
    obj.StoreFix (v06, storage);

    obj.StoreFix (v07, storage);
    obj.StoreFix (v08, storage);
    obj.StoreFix (v09, storage);
    obj.StoreFix (v10, storage);



    // SocketServer oSrv;
    // SocketClient oCli;
    // 
    // oSrv.Start ( CONN_PORT, CONN_TYPE );
    // std::this_thread::sleep_for(30000ms);
    // oCli.Connect ( CONN_PORT, CONN_TYPE );


    while( true ) {
        std::this_thread::sleep_for(20000ms);
    }
}