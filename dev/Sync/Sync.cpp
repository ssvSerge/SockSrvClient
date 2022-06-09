#include <iostream>
#include <thread>
#include <chrono>

#include "SockServerClient.h"
#include "Serializer.h"

// #define CONN_PORT     ("c://temp//sock_conn")
// #define CONN_TYPE     (conn_type_t::CONN_TYPE_FILE)

#define CONN_PORT     ("4567")
#define CONN_TYPE     (conn_type_t::CONN_TYPE_SOCK)

using namespace std::chrono_literals;

void test_fix ( ) {

    int8_t      out_v01 = 0x55;
    uint8_t     out_v02 = 0x56;
    int16_t     out_v03 = 0x595A;
    uint16_t    out_v04 = 0x5758;
    int32_t     out_v05 = 0x5F606162;
    uint32_t    out_v06 = 0x5B5C5D5E;
    int64_t     out_v07 = 0x6B6C6D6E6F707172U;
    uint64_t    out_v08 = 0x636465666768696AU;
    float       out_v09 = static_cast<float> (0.1);
    double      out_v10 = 0.33;
    uint32_t    out_v11 = 0;
    uint32_t    out_v12 = 0x10;
    uint32_t    out_v13 = 0x1011;
    uint32_t    out_v14 = 0x111213;
    uint32_t    out_v15 = 0x11121314;

    int8_t      in_v01 = 0;
    uint8_t     in_v02 = 0;
    int16_t     in_v03 = 0;
    uint16_t    in_v04 = 0;
    int32_t     in_v05 = 0;
    uint32_t    in_v06 = 0;
    int64_t     in_v07 = 0;
    uint64_t    in_v08 = 0;
    float       in_v09 = 0;
    double      in_v10 = 0;
    uint32_t    in_v11 = 0;
    uint32_t    in_v12 = 0;
    uint32_t    in_v13 = 0;
    uint32_t    in_v14 = 0;
    uint32_t    in_v15 = 0;


    hid::Serializer obj_out;
    hid::serializer_storage_t storage_out;

    hid::Serializer obj_in;
    hid::serializer_storage_t storage_in;

    obj_out.StoreFix(out_v01, storage_out);
    obj_out.StoreFix(out_v02, storage_out);
    obj_out.StoreFix(out_v03, storage_out);
    obj_out.StoreFix(out_v04, storage_out);
    obj_out.StoreFix(out_v05, storage_out);
    obj_out.StoreFix(out_v06, storage_out);
    obj_out.StoreFix(out_v07, storage_out);
    obj_out.StoreFix(out_v08, storage_out);
    obj_out.StoreFix(out_v09, storage_out);
    obj_out.StoreFix(out_v10, storage_out);
    obj_out.StoreArr(out_v11, storage_out);
    obj_out.StoreArr(out_v12, storage_out);
    obj_out.StoreArr(out_v13, storage_out);
    obj_out.StoreArr(out_v14, storage_out);
    obj_out.StoreArr(out_v15, storage_out);

    storage_in = storage_out;

    obj_in.LoadFix(storage_out, in_v01 );
    obj_in.LoadFix(storage_out, in_v02 );
    obj_in.LoadFix(storage_out, in_v03 );
    obj_in.LoadFix(storage_out, in_v04 );
    obj_in.LoadFix(storage_out, in_v05 );
    obj_in.LoadFix(storage_out, in_v06 );
    obj_in.LoadFix(storage_out, in_v07 );
    obj_in.LoadFix(storage_out, in_v08 );
    obj_in.LoadFix(storage_out, in_v09 );
    obj_in.LoadFix(storage_out, in_v10 );
    obj_in.LoadArr(storage_out, in_v11 );
    obj_in.LoadArr(storage_out, in_v12 );
    obj_in.LoadArr(storage_out, in_v13 );
    obj_in.LoadArr(storage_out, in_v14 );
    obj_in.LoadArr(storage_out, in_v15 );


    std::cout << (in_v01 == out_v01) << "\r\n";
    std::cout << (in_v02 == out_v02) << "\r\n";
    std::cout << (in_v03 == out_v03) << "\r\n";
    std::cout << (in_v04 == out_v04) << "\r\n";
    std::cout << (in_v05 == out_v05) << "\r\n";
    std::cout << (in_v06 == out_v06) << "\r\n";
    std::cout << (in_v07 == out_v07) << "\r\n";
    std::cout << (in_v08 == out_v08) << "\r\n";
    std::cout << (in_v09 == out_v09) << "\r\n";
    std::cout << (in_v10 == out_v10) << "\r\n";
    std::cout << (in_v11 == out_v11) << "\r\n";
    std::cout << (in_v12 == out_v12) << "\r\n";
    std::cout << (in_v13 == out_v13) << "\r\n";
    std::cout << (in_v14 == out_v14) << "\r\n";
    std::cout << (in_v15 == out_v15) << "\r\n";


}

void test_var() {
}


int main() {

    hid::Serializer obj;
    hid::serializer_storage_t storage;

    test_fix();
    test_var();


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