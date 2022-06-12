#include <ctime>
#include <iostream>
#include <vector>
#include <string>

#include <Serializer.h>

#define  CNT_INT32          ( 66 * 1024 )
#define  CNT_INT16          ( 300 )
#define  CNT_INT8           ( 27 )

class ObjComplex {
    public:
        hid::serializer_string_t param1;
        hid::serializer_string_t param2;
};

typedef std::vector<ObjComplex>   complex_list_t;

template <typename T>
void fill_str ( size_t len, T& var ) {

    var.resize(len);

    for (size_t i = 0; i < var.size(); i++) {
        int r = std::rand() % 26;
        var[i] = static_cast<char> ('a' + r);
    }

}

void fill_cpx ( ObjComplex& ref ) {

    size_t cnt1 = std::rand() % 64 + 1;
    fill_str(cnt1, ref.param1 );

    size_t cnt2 = std::rand() % 64 + 1;
    fill_str(cnt2, ref.param2 );
}

class Sample {
    
    public:
        Sample() {
            input_clear();
            output_clear();
        }

        void input_clear () {
            param_int_01 = -1;
            param_int_02 = static_cast<uint8_t>  (-1);
            param_int_03 = -1;
            param_int_04 = static_cast<uint16_t> (-1);
            param_int_05 = -1;
            param_int_06 = static_cast<uint32_t> (-1);
            param_int_07 = -1;
            param_int_08 = static_cast<uint64_t> (-1);
            param_flo_09 = -1;
            param_flo_10 = -1;
        }

        void input_prepare () {
            param_int_01 = 0x51;
            param_int_02 = 0x52;
            param_int_03 = 0x5354;
            param_int_04 = 0x5556;
            param_int_05 = 0x5758595A;
            param_int_06 = 0x5B5C5D5E;
            param_int_07 = 0x5F60616264656667;
            param_int_08 = 0x68696A6B6C6D6E6F;
            param_flo_09 = static_cast <float> (0.13);
            param_flo_10 = 0.77;
        }

        void output_clear () {
            res_01.clear();
            res_02.clear();
            res_03.clear();
            res_04.clear();
            res_05.clear();
            res_06.clear();
            res_07.clear();
            res_08.clear();
            res_09.clear();
            res_10.clear();
            res_11.clear();
            res_12.clear();
        }

        void output_prepare () {

            fill_str ( 100 * 1024, res_01 );
            fill_str ( 60000, res_02 );
            fill_str ( 200, res_03 );
            fill_str ( 0, res_04 );

            fill_str( 100 * 1024, res_05 );
            fill_str( 60000, res_06 );
            fill_str( 200, res_07 );
            fill_str( 0, res_08 );

            size_t cnt;

            cnt = CNT_INT32;
            for ( size_t i = 0; i < cnt; i++ ) {
                ObjComplex newObj;
                fill_cpx (newObj);
                res_09.push_back (newObj);
            }

            cnt = CNT_INT16;
            for (size_t i = 0; i < cnt; i++) {
                ObjComplex newObj;
                fill_cpx(newObj);
                res_10.push_back(newObj);
            }

            cnt = CNT_INT8;
            for (size_t i = 0; i < cnt; i++) {
                ObjComplex newObj;
                fill_cpx(newObj);
                res_11.push_back(newObj);
            }

            res_12.clear();
        }

    // parameters
    public:
        int8_t    param_int_01;
        uint8_t   param_int_02;
        int16_t   param_int_03;
        uint16_t  param_int_04;
        int32_t   param_int_05;
        uint32_t  param_int_06;
        int64_t   param_int_07;
        uint64_t  param_int_08;
        float     param_flo_09;
        double    param_flo_10;

    // Results
    public:
        hid::serializer_bin_t    res_01;
        hid::serializer_bin_t    res_02;
        hid::serializer_bin_t    res_03;
        hid::serializer_bin_t    res_04;

        hid::serializer_string_t res_05;
        hid::serializer_string_t res_06;
        hid::serializer_string_t res_07;
        hid::serializer_string_t res_08;

        complex_list_t           res_09;
        complex_list_t           res_10;
        complex_list_t           res_11;
        complex_list_t           res_12;

};

bool cmp_list (const complex_list_t& p1, const complex_list_t& p2) {

    if (p1.size() != p2.size()) {
        return false;
    }

    for (size_t i = 0; i < p1.size(); i++) {
        if (p1[i].param1 != p2[i].param1) {
            return false;
        }
        if (p1[i].param2 != p2[i].param2) {
            return false;
        }
    }

    return true;
}

static void serialize_test() {

    hid::serializer_storage_t storage_out;
    hid::serializer_storage_t storage_inp;

    Sample   data_out;
    Sample   data_inp;

    storage_out.reserve (128 * 1024 * 1024);
    storage_inp.reserve (128 * 1024 * 1024);

    data_out.input_prepare();
    data_inp.input_clear();


    {   hid::Serializer serializer;

        serializer.StoreFix ( data_out.param_int_01, storage_out );
        serializer.StoreFix ( data_out.param_int_02, storage_out );
        serializer.StoreFix ( data_out.param_int_03, storage_out );
        serializer.StoreFix ( data_out.param_int_04, storage_out );
        serializer.StoreFix ( data_out.param_int_05, storage_out );
        serializer.StoreFix ( data_out.param_int_06, storage_out );
        serializer.StoreFix ( data_out.param_int_07, storage_out );
        serializer.StoreFix ( data_out.param_int_08, storage_out );
        serializer.StoreFix ( data_out.param_flo_09, storage_out );
        serializer.StoreFix ( data_out.param_flo_10, storage_out );

        std::cout << serializer.ExportStatus();
    }

    {   hid::Serializer serializer;

        serializer.LoadFix ( storage_out, data_inp.param_int_01 );
        serializer.LoadFix ( storage_out, data_inp.param_int_02 );
        serializer.LoadFix ( storage_out, data_inp.param_int_03 );
        serializer.LoadFix ( storage_out, data_inp.param_int_04 );
        serializer.LoadFix ( storage_out, data_inp.param_int_05 );
        serializer.LoadFix ( storage_out, data_inp.param_int_06 );
        serializer.LoadFix ( storage_out, data_inp.param_int_07 );
        serializer.LoadFix ( storage_out, data_inp.param_int_08 );
        serializer.LoadFix ( storage_out, data_inp.param_flo_09 );
        serializer.LoadFix ( storage_out, data_inp.param_flo_10 );

        std::cout << serializer.ImportStatus(storage_out);
    }

    std::cout << (data_out.param_int_01 == data_inp.param_int_01);
    std::cout << (data_out.param_int_02 == data_inp.param_int_02);
    std::cout << (data_out.param_int_03 == data_inp.param_int_03);
    std::cout << (data_out.param_int_04 == data_inp.param_int_04);
    std::cout << (data_out.param_int_05 == data_inp.param_int_05);
    std::cout << (data_out.param_int_06 == data_inp.param_int_06);
    std::cout << (data_out.param_int_07 == data_inp.param_int_07);
    std::cout << (data_out.param_int_08 == data_inp.param_int_08);
    std::cout << (data_out.param_flo_09 == data_inp.param_flo_09);
    std::cout << (data_out.param_flo_10 == data_inp.param_flo_10);

    std::cout << std::endl;

    data_inp.output_prepare();

    {   hid::Serializer serializer;
        serializer.StoreVar ( data_inp.res_01, storage_inp );
        serializer.StoreVar ( data_inp.res_02, storage_inp );
        serializer.StoreVar ( data_inp.res_03, storage_inp );
        serializer.StoreVar ( data_inp.res_04, storage_inp );
        serializer.StoreVar ( data_inp.res_05, storage_inp );
        serializer.StoreVar ( data_inp.res_06, storage_inp );
        serializer.StoreVar ( data_inp.res_07, storage_inp );
        serializer.StoreVar ( data_inp.res_08, storage_inp );

        serializer.StoreCnt (data_inp.res_09.size(), storage_inp);
        for ( size_t i = 0; i < data_inp.res_09.size(); i++ ) {
            serializer.StoreVar ( data_inp.res_09[i].param1, storage_inp );
            serializer.StoreVar ( data_inp.res_09[i].param2, storage_inp );
        }

        serializer.StoreCnt (data_inp.res_10.size(), storage_inp);
        for ( size_t i = 0; i < data_inp.res_10.size(); i++ ) {
            serializer.StoreVar ( data_inp.res_10[i].param1, storage_inp );
            serializer.StoreVar ( data_inp.res_10[i].param2, storage_inp );
        }

        serializer.StoreCnt (data_inp.res_11.size(), storage_inp);
        for ( size_t i = 0; i < data_inp.res_11.size(); i++ ) {
            serializer.StoreVar ( data_inp.res_11[i].param1, storage_inp );
            serializer.StoreVar ( data_inp.res_11[i].param2, storage_inp );
        }

        serializer.StoreCnt (data_inp.res_12.size(), storage_inp);
        for ( size_t i = 0; i < data_inp.res_12.size(); i++ ) {
            serializer.StoreVar ( data_inp.res_12[i].param1, storage_inp );
            serializer.StoreVar ( data_inp.res_12[i].param2, storage_inp );
        }
    }

    {   hid::Serializer serializer;

        serializer.LoadVar ( storage_inp, data_out.res_01 );
        serializer.LoadVar ( storage_inp, data_out.res_02 );
        serializer.LoadVar ( storage_inp, data_out.res_03 );
        serializer.LoadVar ( storage_inp, data_out.res_04 );
        serializer.LoadVar ( storage_inp, data_out.res_05 );
        serializer.LoadVar ( storage_inp, data_out.res_06 );
        serializer.LoadVar ( storage_inp, data_out.res_07 );
        serializer.LoadVar ( storage_inp, data_out.res_08 );

        size_t cnt;

        serializer.LoadCnt (storage_inp, cnt);
        for (size_t i = 0; i < cnt; i++) {
            ObjComplex newObj;
            serializer.LoadVar(storage_inp, newObj.param1);
            serializer.LoadVar(storage_inp, newObj.param2);
            data_out.res_09.emplace_back ( std::move(newObj) );
        }

        serializer.LoadCnt(storage_inp, cnt);
        for (size_t i = 0; i < cnt; i++) {
            ObjComplex newObj;
            serializer.LoadVar(storage_inp, newObj.param1);
            serializer.LoadVar(storage_inp, newObj.param2);
            data_out.res_10.emplace_back(std::move(newObj));
        }

        serializer.LoadCnt(storage_inp, cnt);
        for (size_t i = 0; i < cnt; i++) {
            ObjComplex newObj;
            serializer.LoadVar(storage_inp, newObj.param1);
            serializer.LoadVar(storage_inp, newObj.param2);
            data_out.res_11.emplace_back(std::move(newObj));
        }

        serializer.LoadCnt(storage_inp, cnt);
        for (size_t i = 0; i < cnt; i++) {
            ObjComplex newObj;
            serializer.LoadVar(storage_inp, newObj.param1);
            serializer.LoadVar(storage_inp, newObj.param2);
            data_out.res_12.emplace_back(std::move(newObj));
        }

    }

    std::cout << (data_out.res_01 == data_inp.res_01);
    std::cout << (data_out.res_02 == data_inp.res_02);
    std::cout << (data_out.res_03 == data_inp.res_03);
    std::cout << (data_out.res_04 == data_inp.res_04);
    std::cout << (data_out.res_05 == data_inp.res_05);
    std::cout << (data_out.res_06 == data_inp.res_06);
    std::cout << (data_out.res_07 == data_inp.res_07);
    std::cout << (data_out.res_08 == data_inp.res_08);
    
    std::cout << cmp_list (data_out.res_09, data_inp.res_09);
    std::cout << cmp_list (data_out.res_10, data_inp.res_10);
    std::cout << cmp_list (data_out.res_11, data_inp.res_11);
    std::cout << cmp_list (data_out.res_12, data_inp.res_12);

    std::cout << std::endl;
}

static void serialize_wrong() {


    int                         param_int = 0x01;
    hid::serializer_string_t    param_str = "abc";
    ObjComplex                  param_obj;
    complex_list_t              param_list;

    {   hid::Serializer serializer;
        hid::serializer_storage_t storage;
        serializer.StoreFix(param_int, storage);    // OK
        serializer.StoreFix(param_str, storage);    // Cannot store STR as Integral. Failed.
        std::cout << ( ! serializer.ExportStatus() );
    }

    {   hid::Serializer serializer;
        hid::serializer_storage_t storage;
        serializer.StoreFix(param_int, storage);    // OK
        serializer.StoreFix(param_obj, storage);    // Cannot store OBJ as Integral. Failed.
        std::cout << ( ! serializer.ExportStatus() );
    }

    {   hid::Serializer serializer;
        hid::serializer_storage_t storage;
        serializer.StoreVar(param_str,  storage);   // OK
        serializer.StoreVar(param_list, storage);   // Unsupported type.  Failed.
        std::cout << ( ! serializer.ExportStatus() );
    }

    {   hid::Serializer serializer_out;
        hid::Serializer serializer_inp;
        hid::serializer_storage_t storage;
        size_t p1 = 100;
        char   c1 = 0;

        serializer_out.StoreFix ( p1, storage ); // Ok
        std::cout << serializer_out.ExportStatus();

        serializer_inp.LoadFix  ( storage, c1 ); // Stored <size_t> but Requested <char>. Failed.
        std::cout << ( ! serializer_inp.ExportStatus() );
    }


    {   hid::Serializer serializer_out;
        hid::Serializer serializer_inp;
        hid::serializer_storage_t storage;
        hid::serializer_string_t p1 = "abcd";
        hid::serializer_bin_t    p2;

        serializer_out.StoreVar ( p1, storage ); // Ok
        std::cout << serializer_out.ExportStatus();

        serializer_inp.LoadVar  ( storage, p2 ); // Stored as <STR>, Load as <BIN>. Failed.
        std::cout << ( ! serializer_inp.ExportStatus() );
    }

    {   hid::Serializer serializer_out;
        hid::Serializer serializer_inp;
        hid::serializer_storage_t storage;
        hid::serializer_bin_t    p1;
        hid::serializer_string_t p2 = "abcd";

        serializer_out.StoreVar ( p1, storage ); // Ok
        std::cout << serializer_out.ExportStatus();

        serializer_inp.LoadVar  ( storage, p2 ); // Stored as <BIN>, Load as <STR>. Failed.
        std::cout << ( ! serializer_inp.ExportStatus() );
    }


    {   hid::Serializer serializer_out;
        hid::Serializer serializer_inp;
        hid::serializer_storage_t storage;
        size_t  p1 = 100;
        size_t  p2 = 0;

        serializer_out.StoreCnt ( p1, storage ); // Ok
        std::cout << serializer_out.ExportStatus();

        serializer_inp.LoadFix ( storage, p2 ); // Stored as <CNT>, Load as <INT>. Failed.
        std::cout << ( ! serializer_inp.ExportStatus() );
    }

    {   hid::Serializer serializer_out;
        hid::Serializer serializer_inp;
        hid::serializer_storage_t storage;
        size_t  p1 = 100;
        size_t  p2 = 0;

        serializer_out.StoreFix ( p1, storage ); // Ok
        std::cout << serializer_out.ExportStatus();

        serializer_inp.LoadCnt ( storage, p2 ); // Stored as <CNT>, Load as <INT>. Failed.
        std::cout << ( ! serializer_inp.ExportStatus() );
    }

    {   hid::Serializer serializer_out;
        hid::Serializer serializer_in;
        hid::serializer_storage_t   storage;
        hid::serializer_string_t    p1 = "abcd";
        hid::serializer_string_t    p2 = "1234";
        hid::serializer_string_t    p3;

        serializer_out.StoreVar ( p1, storage );
        serializer_out.StoreVar ( p2, storage );
        std::cout << serializer_out.ExportStatus(); // OK
        serializer_in.LoadVar ( storage, p3 );
        std::cout << ( ! serializer_in.ImportStatus(storage) );  // Stored 2 parameters; Load 2. Failed.
    }

    std::cout << std::endl;
}

static void serialize_cnt() {

    {   hid::Serializer serializer_out;
        hid::Serializer serializer_in;
        hid::serializer_storage_t storage;
        int p1 = 0, p2 = 0;
        int r1 = 0, r2 = 0, r3 = 0;

        serializer_out.StoreFix(p1, storage);  
        serializer_out.StoreFix(p2, storage);  // OK
        std::cout << ( serializer_out.ExportStatus() );
        serializer_in.LoadFix(storage, r1);
        serializer_in.LoadFix(storage, r2);
        serializer_in.LoadFix(storage, r3);    // Requesting 3 parameters while 2 stored. Failed.
        std::cout << ( !serializer_in.ImportStatus(storage) );
    }


    {   hid::Serializer serializer_out;
        hid::Serializer serializer_in;
        hid::serializer_storage_t storage;
        hid::serializer_string_t p1, p2;
        hid::serializer_string_t r1, r2, r3;

        serializer_out.StoreVar(p1, storage);  
        serializer_out.StoreVar(p2, storage);  // OK
        std::cout << ( serializer_out.ExportStatus() );
        serializer_in.LoadVar(storage, r1);
        serializer_in.LoadVar(storage, r2);
        serializer_in.LoadVar(storage, r3);    // Requesting 3 parameters while 2 stored. Failed.
        std::cout << ( !serializer_in.ImportStatus(storage) );
    }

    {   hid::Serializer serializer_out;
        hid::Serializer serializer_in;
        hid::serializer_storage_t storage;
        size_t p1 = 100, p2 = 888;
        size_t r1, r2, r3;

        serializer_out.StoreCnt(p1, storage);
        serializer_out.StoreCnt(p2, storage);  // OK
        std::cout << (serializer_out.ExportStatus());
        serializer_in.LoadCnt(storage, r1);
        serializer_in.LoadCnt(storage, r2);
        serializer_in.LoadCnt(storage, r3);    // Requesting 3 parameters while 2 stored. Failed.
        std::cout << (!serializer_in.ImportStatus(storage));
    }

    {   hid::Serializer serializer_out;
        hid::Serializer serializer_in;
        hid::serializer_storage_t storage;
        size_t p1 = 100, p2 = 888, p3 = 666;
        size_t r1, r2;

        serializer_out.StoreCnt(p1, storage);
        serializer_out.StoreCnt(p2, storage);  // OK
        serializer_out.StoreCnt(p3, storage);  // OK
        std::cout << (serializer_out.ExportStatus());
        serializer_in.LoadCnt(storage, r1);
        serializer_in.LoadCnt(storage, r2);
        std::cout << (!serializer_in.ImportStatus(storage)); // Failed. Stored 3, load 2.
    }

    std::cout << std::endl;
}

int main() {

    std::srand( (unsigned int)std::time(nullptr) );

    serialize_test();
    serialize_wrong();
    serialize_cnt();

    return 0;
}
