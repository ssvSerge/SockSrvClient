#include <cstdlib>
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

class dummy {
    public:
        int i = 0;
        int y = 0;
};


template <typename T>
void fill_str ( size_t len, T& var ) {

    var.resize ( len );

    for ( size_t i = 0; i < var.size (); i++ ) {
        int r = std::rand() % 26;
        var [i] = static_cast<char> ('a' + r);
    }

}

void test_fix ( ) {

    int8_t      in_v01 = 0, out_v01 = 0x55;
    uint8_t     in_v02 = 0, out_v02 = 0x56;
    int16_t     in_v03 = 0, out_v03 = 0x5758;
    uint16_t    in_v04 = 0, out_v04 = 0x595A;
    int32_t     in_v05 = 0, out_v05 = 0x5B5C5D5E;
    uint32_t    in_v06 = 0, out_v06 = 0x5F606162;
    int64_t     in_v07 = 0, out_v07 = 0x636465666768696AU;
    uint64_t    in_v08 = 0, out_v08 = 0x6B6C6D6E6F707172U;
    float       in_v09 = 0, out_v09 = static_cast<float> (0.1);
    double      in_v10 = 0, out_v10 = 0.33;
    uint32_t    in_v11 = 0, out_v11 = 0;
    uint32_t    in_v12 = 0, out_v12 = 0x10;
    uint32_t    in_v13 = 0, out_v13 = 0x1011;
    uint32_t    in_v14 = 0, out_v14 = 0x111213;
    uint32_t    in_v15 = 0, out_v15 = 0x11121314;

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
    obj_out.StoreCnt(out_v11, storage_out);
    obj_out.StoreCnt(out_v12, storage_out);
    obj_out.StoreCnt(out_v13, storage_out);
    obj_out.StoreCnt(out_v14, storage_out);
    obj_out.StoreCnt(out_v15, storage_out);

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
    obj_in.LoadCnt(storage_out, in_v11 );
    obj_in.LoadCnt(storage_out, in_v12 );
    obj_in.LoadCnt(storage_out, in_v13 );
    obj_in.LoadCnt(storage_out, in_v14 );
    obj_in.LoadCnt(storage_out, in_v15 );


    std::cout << (in_v01 == out_v01);
    std::cout << (in_v02 == out_v02);
    std::cout << (in_v03 == out_v03);
    std::cout << (in_v04 == out_v04);
    std::cout << (in_v05 == out_v05);
    std::cout << (in_v06 == out_v06);
    std::cout << (in_v07 == out_v07);
    std::cout << (in_v08 == out_v08);
    std::cout << (in_v09 == out_v09);
    std::cout << (in_v10 == out_v10);
    std::cout << (in_v11 == out_v11);
    std::cout << (in_v12 == out_v12);
    std::cout << (in_v13 == out_v13);
    std::cout << (in_v14 == out_v14);
    std::cout << (in_v15 == out_v15) << "\r\n";
}

void test_str() {

    hid::Serializer obj_out;
    hid::Serializer obj_in;

    hid::serializer_storage_t storage_out;
    hid::serializer_storage_t storage_in;

    hid::serializer_string_t  str_out_01, str_in_01;
    hid::serializer_string_t  str_out_02, str_in_02;
    hid::serializer_string_t  str_out_03, str_in_03;
    hid::serializer_string_t  str_out_04, str_in_04;
    hid::serializer_string_t  str_out_05, str_in_05;

    fill_str (32, str_in_01);
    fill_str (32, str_in_02);
    fill_str (32, str_in_03);
    fill_str (32, str_in_04);
    fill_str (32, str_in_05);

    fill_str (8, str_out_02);
    fill_str (300, str_out_03);
    fill_str (1024, str_out_04);
    fill_str (70*1024, str_out_05);

    obj_out.StoreVar ( str_out_01, storage_out );
    obj_out.StoreVar ( str_out_02, storage_out );
    obj_out.StoreVar ( str_out_03, storage_out );
    obj_out.StoreVar ( str_out_04, storage_out );
    obj_out.StoreVar ( str_out_05, storage_out );

    storage_in = storage_out;

    obj_in.LoadVar (storage_in, str_in_01);
    obj_in.LoadVar (storage_in, str_in_02);
    obj_in.LoadVar (storage_in, str_in_03);
    obj_in.LoadVar (storage_in, str_in_04);
    obj_in.LoadVar (storage_in, str_in_05);

    auto res1 = (str_out_01 == str_in_01);
    auto res2 = (str_out_02 == str_in_02);
    auto res3 = (str_out_03 == str_in_03);
    auto res4 = (str_out_04 == str_in_04);
    auto res5 = (str_out_05 == str_in_05);
    auto res6 = obj_in.Staus (storage_in);

    std::cout << res1 << res2 << res3 << res4 << res5 << res6 << "\r\n";
}

void test_bin() {

    hid::Serializer obj_out;
    hid::Serializer obj_in;

    hid::serializer_storage_t storage_out;
    hid::serializer_storage_t storage_in;

    hid::serializer_bin_t  str_out_01, str_in_01;
    hid::serializer_bin_t  str_out_02, str_in_02;
    hid::serializer_bin_t  str_out_03, str_in_03;
    hid::serializer_bin_t  str_out_04, str_in_04;
    hid::serializer_bin_t  str_out_05, str_in_05;

    fill_str (32, str_in_01);
    fill_str (32, str_in_02);
    fill_str (32, str_in_03);
    fill_str (32, str_in_04);
    fill_str (32, str_in_05);

    fill_str (8, str_out_02);
    fill_str (300, str_out_03);
    fill_str (1024, str_out_04);
    fill_str (70*1024, str_out_05);


    obj_out.StoreVar ( str_out_01, storage_out );
    obj_out.StoreVar ( str_out_02, storage_out );
    obj_out.StoreVar ( str_out_03, storage_out );
    obj_out.StoreVar ( str_out_04, storage_out );
    obj_out.StoreVar ( str_out_05, storage_out );

    storage_in = storage_out;

    obj_in.LoadVar (storage_in, str_in_01);
    obj_in.LoadVar (storage_in, str_in_02);
    obj_in.LoadVar (storage_in, str_in_03);
    obj_in.LoadVar (storage_in, str_in_04);
    obj_in.LoadVar (storage_in, str_in_05);

    auto res1 = (str_out_01 == str_in_01);
    auto res2 = (str_out_02 == str_in_02);
    auto res3 = (str_out_03 == str_in_03);
    auto res4 = (str_out_04 == str_in_04);
    auto res5 = (str_out_05 == str_in_05);
    auto res6 = obj_in.Staus (storage_in);

    std::cout << res1 << res2 << res3 << res4 << res5 << res6 << "\r\n";
}

int main() {

    std::srand(std::time(nullptr));

    hid::Serializer obj;
    hid::serializer_storage_t storage;

    test_fix();
    test_str();
    test_bin();


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