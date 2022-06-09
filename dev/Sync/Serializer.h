#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <stdint.h>

#include <vector>
#include <string>
#include <sstream>

#define UNREF(x)   (void)(x)


namespace hid {

    // Reader and Writer must be synchronized with the length of variables
    // Assume we're on the 64 bit CPU architecture.
    static_assert ( sizeof(uint8_t)     == 1, "Wrong size of uint8_t" );
    static_assert ( sizeof(uint16_t)    == 2, "Wrong size of uint16_t" );
    static_assert ( sizeof(uint32_t)    == 4, "Wrong size of uint32_t" );
    static_assert ( sizeof(uint64_t)    == 8, "Wrong size of uint64_t" );
    static_assert ( sizeof(int)         == 4, "Wrong size of int" );
    static_assert ( sizeof(long)        == 4, "Wrong size of long" );
    static_assert ( sizeof(long long)   == 8, "Wrong size of long long" );
    static_assert ( sizeof(size_t)      == 8, "Wrong size of size_t" );
    static_assert ( sizeof(float)       == 4, "Wrong size of float" );
    static_assert ( sizeof(double)      == 8, "Wrong size of double" );

    typedef std::string             serializer_string_t;
    typedef std::vector<uint8_t>    serializer_bin_t;
    typedef std::vector<uint8_t>    serializer_storage_t;

    enum class serializer_state_t {
        SERIALIZER_STATE_UNKNOWN,
        SERIALIZER_STATE_OK,
        SERIALIZER_STATE_FAILED
    };

    enum class serializer_len_t: uint32_t {
        SERIALIZER_LEN_ZERO
    };

    enum class serializer_type_t : uint8_t {

        SERIALIZER_TYPE_UNDEFINED           = 0,

        SERIALIZER_TYPE_INT8                = 0x11,
        SERIALIZER_TYPE_UINT8               = 0x12,
        SERIALIZER_TYPE_INT16               = 0x13,
        SERIALIZER_TYPE_UINT16              = 0x14,
        SERIALIZER_TYPE_INT32               = 0x15,
        SERIALIZER_TYPE_UINT32              = 0x16,
        SERIALIZER_TYPE_INT64               = 0x17,
        SERIALIZER_TYPE_UINT64              = 0x18,
        SERIALIZER_TYPE_FLOAT               = 0x19,
        SERIALIZER_TYPE_DOUBLE              = 0x1A,

        SERIALIZER_TYPE_VECTOR_SIZE_ZERO    = 0x20,
        SERIALIZER_TYPE_VECTOR_SIZE_BYTE    = 0x21,
        SERIALIZER_TYPE_VECTOR_SIZE_WORD    = 0x22,
        SERIALIZER_TYPE_VECTOR_SIZE_DWORD   = 0x24,

        SERIALIZER_TYPE_STRING_SIZE_ZERO    = 0x30,
        SERIALIZER_TYPE_STRING_SIZE_BYTE    = 0x31,
        SERIALIZER_TYPE_STRING_SIZE_WORD    = 0x32,
        SERIALIZER_TYPE_STRING_SIZE_DWORD   = 0x34,

        SERIALIZER_TYPE_ARR_SIZE_ZERO       = 0x40,
        SERIALIZER_TYPE_ARR_SIZE_BYTE       = 0x41,
        SERIALIZER_TYPE_ARR_SIZE_WORD       = 0x42,
        SERIALIZER_TYPE_ARR_SIZE_DWORD      = 0x43,

        SERIALIZER_TYPE_TYPE                = 0x50,
        SERIALIZER_TYPE_LASTID
    };

    class Serializer {

        public:
            Serializer() {
                Reset();
            }

        public:
            void Reset () {
                m_state     = serializer_state_t::SERIALIZER_STATE_OK;
                m_offset    = 0;
                m_error_pos = 0;
            }

        public:

            // Supported types (or compatible): 
            //     int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t
            // 
            // Result:
            //     Extend storage and put the <TYPE_ID> + <VALUE>.
            //     <TYPE_ID>   => Fixed length (1 byte). 
            //     <VALUE>     => Variadic length (depends on TYPE_ID). Could be 1, 2, 4 or 8 bytes.
            // 
            // Storage:
            //     [   SERIALIZER_TYPE_INT8 ]   [value]
            //     [  SERIALIZER_TYPE_UINT8 ]   [value]
            //     [  SERIALIZER_TYPE_INT16 ]   [value_lo][value_hi]
            //     [ SERIALIZER_TYPE_UINT16 ]   [value_lo][value_hi]
            //     [  SERIALIZER_TYPE_INT32 ]   [value_lo][2][3][value_hi]
            //     [ SERIALIZER_TYPE_UINT32 ]   [value_lo][2][3][value_hi]
            //     [  SERIALIZER_TYPE_INT64 ]   [value_lo][2][3][4][5][6][7][value_hi]
            //     [ SERIALIZER_TYPE_UINT64 ]   [value_lo][2][3][4][5][6][7][value_hi]
            //
            template<typename T>
            void StoreFix ( const T& var, serializer_storage_t& storage ) {

                if ( can_continue() ) {

                    try {

                        serializer_type_t var_type;
                        size_t            var_len;

                        get_type(var, var_type, var_len);

                        switch( var_type ) {
                            case serializer_type_t::SERIALIZER_TYPE_INT8:
                            case serializer_type_t::SERIALIZER_TYPE_UINT8:
                            case serializer_type_t::SERIALIZER_TYPE_INT16:
                            case serializer_type_t::SERIALIZER_TYPE_UINT16:
                            case serializer_type_t::SERIALIZER_TYPE_INT32:
                            case serializer_type_t::SERIALIZER_TYPE_UINT32:
                            case serializer_type_t::SERIALIZER_TYPE_INT64:
                            case serializer_type_t::SERIALIZER_TYPE_UINT64:
                            case serializer_type_t::SERIALIZER_TYPE_FLOAT:
                            case serializer_type_t::SERIALIZER_TYPE_DOUBLE:
                                break;

                            default:

                                m_error_msg << "Unsupported type of variable. Expected (U)INT. Received: <" << typeid(var).name() << ">";
                                m_error_pos  = m_offset;
                                m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;

                                break;
                        }

                        store ( &var_type, sizeof(var_type), storage );
                        store ( &var, var_len, storage );

                    } catch ( ... ) {
                        m_error_msg << "Internal error in: " << __FUNCTION__;
                        m_error_pos  = m_offset;
                        m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;
                        m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                }

            }

            // Supported types (or compatible): 
            //     std::vector<uint8_t>, std::vector<char>, std::string
            // 
            // Result:
            //     Extend storage and put the <TYPE_ID> + <LEN> + <PAYLOAD>.
            //     <TYPE_ID>   => Fixed length (1 byte). 
            //     <LEN>       => Variadic length (depends on TYPE_ID). Could be 0, 1, 2 or 4 bytes.
            //     <PAYLOAD>   => Variadic length (depends on LEN).
            // 
            // Storage:
            //     Zero length (empty) arrays. No Length; No Payload.
            //     [  SERIALIZER_TYPE_VECTOR_SIZE_ZERO ]
            //     [  SERIALIZER_TYPE_STRING_SIZE_ZERO ]
            // 
            //     Short arrays (from 1 to 255 bytes). [LEN] is 1 bytes; Payload from 1 to 255 bytes.
            //     1 byte length; Payload up to 255 bytes;
            //     [  SERIALIZER_TYPE_VECTOR_SIZE_BYTE ]   [LEN]                    [Payload begin] ... [Payload end]
            //     [  SERIALIZER_TYPE_STRING_SIZE_BYTE ]   [LEN]                    [Payload begin] ... [Payload end]
            // 
            //     Arrays (from 255 to 65535 bytes). [LEN] is 2 bytes; Payload from 256 to 65535 bytes.
            //     [  SERIALIZER_TYPE_VECTOR_SIZE_WORD ]   [LEN_LO][LEN_HI]         [Payload begin] ... [Payload end]
            //     [  SERIALIZER_TYPE_STRING_SIZE_WORD ]   [LEN_LO][LEN_HI]         [Payload begin] ... [Payload end]
            // 
            //     Large arrays (from 65536 to 4 GB); [LEN] is 4 bytes; Payload from 65536 to 4GB.
            //     [ SERIALIZER_TYPE_VECTOR_SIZE_DWORD ]   [LEN_LO][1][2][LEN_HI]   [Payload begin] ... [Payload end]
            //     [ SERIALIZER_TYPE_STRING_SIZE_DWORD ]   [LEN_LO][1][2][LEN_HI]   [Payload begin] ... [Payload end]
            // 
            template<typename T>
            void StoreVar ( const T& var, serializer_storage_t& storage ) {

                if ( can_continue() ) {

                    try {

                        serializer_type_t var_type = serializer_type_t::SERIALIZER_TYPE_UNDEFINED;
                        size_t            payload_len = 0;
                        size_t            payload_len_len = 0;
                        size_t            unused = 0;

                        get_type ( var, var_type, unused );

                        switch ( var_type ) {

                            case serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_DWORD:

                                payload_len = var.size();

                                if ( payload_len == 0 ) {
                                    payload_len_len = 0;
                                    var_type = serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_ZERO;
                                } else
                                if ( payload_len <= 255 ) {
                                    payload_len_len = sizeof(uint8_t);
                                    var_type = serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_BYTE;
                                } else
                                if ( payload_len <= 65535 ) {
                                    payload_len_len = sizeof(uint16_t);
                                    var_type = serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_WORD;
                                } else {
                                    payload_len_len = sizeof(uint32_t);
                                }

                                break;

                            case serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_DWORD:

                                payload_len = var.size();

                                if ( payload_len == 0 ) {
                                    payload_len_len = 0;
                                    var_type = serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_ZERO;
                                } else
                                if ( payload_len <= 255 ) {
                                    payload_len_len = sizeof(uint8_t);
                                    var_type = serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_BYTE;
                                } else
                                if ( payload_len <= 65535 ) {
                                    payload_len_len = sizeof(uint16_t);
                                    var_type = serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_WORD;
                                } else {
                                    payload_len_len = sizeof(uint32_t);
                                }

                                break;

                            default:

                                m_error_msg << "Unsupported type of variable. Expected vector<char> or vector<uchar>. Received: <" << typeid(var).name() << ">";
                                m_error_pos  = m_offset;
                                m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;

                                break;
                        }

                        store ( &var_type, sizeof(var_type), storage );

                        if ( payload_len > 0 ) {
                            store ( &payload_len, payload_len_len, storage );
                            store (var.data(), payload_len, storage );
                        }

                    } catch( ... ) {
                        m_error_msg << "Internal error in: " << __FUNCTION__;
                        m_error_pos  = m_offset;
                        m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                }
            }

            // Supported types (or compatible):
            //     int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t
            // 
            // Result: 
            //     Store the specific value <repetition counter> to storage. Stored <TYPE> <CNT>
            //     <TYPE_ID>   => Fixed length (1 byte). 
            //     <CNT>       => Variadic length (depends on TYPE_ID). Could be 0, 1, 2 or 4 bytes.
            // 
            // Storage:
            //     Zero length (empty) counter. 
            //     [  SERIALIZER_TYPE_ARR_SIZE_ZERO ]
            //     Repetition from 1 to 255. 
            //     [  SERIALIZER_TYPE_ARR_SIZE_BYTE ]      [CNT]
            //     [  SERIALIZER_TYPE_ARR_SIZE_WORD ]      [CNT_LO][CNT_HI]
            //     [ SERIALIZER_TYPE_ARR_SIZE_DWORD ]      [CNT_LO][1][2][CNT_HI]
            //
            void StoreCnt ( uint32_t cnt, serializer_storage_t& storage ) {

                if ( can_continue() ) {

                    try {

                        serializer_type_t var_type = serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_DWORD;
                        size_t            var_type_len = sizeof ( uint32_t );

                        if ( cnt <= 65535 ) {
                            var_type = serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_WORD;
                            var_type_len = sizeof ( uint16_t );
                        }
                        if ( cnt <= 255 ) {
                            var_type = serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_BYTE;
                            var_type_len = sizeof ( uint8_t );
                        }
                        if ( cnt == 0 ) {
                            var_type = serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_ZERO;
                            var_type_len = 0;
                        }

                        store ( &var_type, sizeof ( var_type ), storage );

                        if( var_type_len > 0 ) {
                            store ( &cnt, var_type_len, storage );
                        }

                    } catch( ... ) {
                        m_error_msg << "Internal error in: " << __FUNCTION__;
                        m_error_pos  = m_offset;
                        m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                }
            }

        public:

            // Supported types (or compatible): 
            //     int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t
            // 
            // Result:
            //     Load <TYPE_ID> + <VALUE> from storage.
            //     <TYPE_ID>   => Fixed length (1 byte). 
            //     <VALUE>     => Variadic length (depends on TYPE_ID). Could be 1, 2, 4 or 8 bytes.
            // 
            template<typename T>
            void LoadFix ( const serializer_storage_t& storage, T& var ) {

                var = 0;

                if ( can_continue() ) {

                    try {
                        size_t              exp_len = 0;
                        serializer_type_t   exp_type = serializer_type_t::SERIALIZER_TYPE_LASTID;
                        serializer_type_t   load_type = serializer_type_t::SERIALIZER_TYPE_LASTID;

                        get_type ( var, exp_type, exp_len );

                        load ( storage, &load_type, sizeof ( load_type ) );

                        if( exp_type != load_type ) {

                            m_error_msg << __FUNCTION__ << "; Wrong type of variable. Expected: " << typeid(load_type).name () << "; Received: " << typeid(load_type).name ();
                            m_error_pos = m_offset;
                            m_state = serializer_state_t::SERIALIZER_STATE_FAILED;

                        } else {
                            load ( storage, &var, exp_len );
                        }
                    } catch( ... ) {
                        m_error_msg << "Internal error in: " << __FUNCTION__;
                        m_error_pos  = m_offset;
                        m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                }
            }

            // Supported types (or compatible): 
            //     std::vector<uint8_t>, std::vector<char>, std::string
            // 
            template<typename T>
            void LoadVar ( const serializer_storage_t& storage, T& var ) {

                var.clear ();

                if ( can_continue () ) {

                    try {

                        size_t              payload_len_len = 0;
                        size_t              payload_len = 0;
                        size_t              unused;
                        bool                is_failed = true;
                        serializer_type_t   exp_type;
                        serializer_type_t   load_type;

                        get_type ( var, exp_type, unused );

                        load ( storage, &load_type, sizeof ( load_type ) );

                        if ( exp_type == serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_DWORD ) {

                            switch ( load_type ) {
                                case serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_DWORD:
                                    is_failed = false;
                                    payload_len_len = sizeof ( uint32_t );
                                    break;
                                case serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_WORD:
                                    is_failed = false;
                                    payload_len_len = sizeof ( uint16_t );
                                    break;
                                case serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_BYTE:
                                    is_failed = false;
                                    payload_len_len = sizeof ( uint8_t );
                                    break;
                                case serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_ZERO:
                                    is_failed = false;
                                    payload_len_len = 0;
                                    break;
                                default:
                                    m_error_msg << "Unexpected type received. Expected: <TYPE_VECTOR>; Received: " << (int) load_type;
                                    m_error_pos = m_offset;
                                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                                    break;
                            }

                        } else

                        if ( exp_type == serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_DWORD ) {

                            switch ( load_type ) {
                                case serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_DWORD:
                                    is_failed = false;
                                    payload_len_len = sizeof ( uint32_t );
                                    break;
                                case serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_WORD:
                                    is_failed = false;
                                    payload_len_len = sizeof ( uint16_t );
                                    break;
                                case serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_BYTE:
                                    is_failed = false;
                                    payload_len_len = sizeof ( uint8_t );
                                    break;
                                case serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_ZERO:
                                    is_failed = false;
                                    payload_len_len = 0;
                                    break;
                                default:
                                    m_error_msg << "Unexpected type received. Expected: <TYPE_STRING>; Received: " << (int) load_type;
                                    m_error_pos = m_offset;
                                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                                    break;
                            }
                        }

                        if ( is_failed ) {

                            m_error_msg << __FUNCTION__ << "; Wrong type of variable. Expected: " << typeid(load_type).name () << "; Received: " << typeid(load_type).name ();
                            m_error_pos = m_offset;
                            m_state = serializer_state_t::SERIALIZER_STATE_FAILED;

                        } else {

                            if ( payload_len_len > 0 ) {
                                load ( storage, &payload_len, payload_len_len );
                                if( can_continue () ) {
                                    var.resize ( payload_len );
                                    load ( storage, var.data (), payload_len );
                                }
                            }

                        }

                    } catch ( ... ) {
                        m_error_msg << "Internal error in: " << __FUNCTION__;
                        m_error_pos  = m_offset;
                        m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }
                }   
            }

            // Supported types (or compatible):
            //     int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t
            // 
            // Result: 
            //     Load the specific value <repetition counter> to storage. Stored <TYPE> <CNT>
            // 
            void LoadCnt ( const serializer_storage_t& storage, uint32_t& cnt ) {

                cnt = 0;

                if ( can_continue () ) {

                    try {

                        serializer_type_t var_type;
                        size_t            arr_len_len = 0;
                        uint32_t          arr_len     = 0;

                        load ( storage, &var_type, sizeof(var_type) );

                        switch (var_type) {
                            case serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_ZERO:
                                arr_len_len = 0;
                                break;
                            case serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_BYTE:
                                arr_len_len = 1;
                                break;
                            case serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_WORD:
                                arr_len_len = 2;
                                break;
                            case serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_DWORD:
                                arr_len_len = 4;
                                break;
                            default:
                                // TRACE_ERROR ("Wrong type. Expected: %d; Received: %d", vt, st);
                                m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                                break;
                        }

                        if (arr_len_len > 0) {
                            load ( storage, &arr_len, arr_len_len );
                            if (can_continue()) {
                                cnt = arr_len;
                            }
                        }

                    } catch ( ... ) {
                        m_error_msg << "Internal error in: " << __FUNCTION__;
                        m_error_pos  = m_offset;
                        m_state      = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                }
            }

        public:
            bool Staus ( const serializer_storage_t& storage ) {
                
                if ( m_state != serializer_state_t::SERIALIZER_STATE_OK ) {
                    return false;
                }
                return (storage.size () == m_offset);
            }

        private:
            bool can_continue () {
                return (m_state == serializer_state_t::SERIALIZER_STATE_OK);
            }

            void check_size ( serializer_storage_t& storage, size_t len ) {
                try {
                    size_t my_capacity = storage.capacity();
                    if ( my_capacity < (m_offset + len + 32) ) {
                        storage.reserve ( storage.size () + len + 1024 );
                    }
                    storage.resize ( storage.size()+len );
                } catch( ... ) {
                    // TRACE_ERROR ( "Exception" );
                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                }
            }

            void store ( const void* const value, size_t len, serializer_storage_t& storage ) {

                try {

                    if ( can_continue() ) {
                        const uint8_t* const bin = static_cast<const uint8_t*> (value);
                        check_size ( storage, len );
                        memcpy ( storage.data()+m_offset, bin, len );
                        m_offset += len;
                    }

                } catch( ... ) {
                    // TRACE_ERROR ( "Exception" );
                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                }
            }

            void load  ( const serializer_storage_t& storage, void* const value, size_t len) {
                if ( (m_offset+len)  > storage.size() ) {
                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                } else {
                    memcpy(value, storage.data() + m_offset, len);
                    m_offset += len;
                }
            }
            
        private:
            template<typename T>
            static void get_type ( const T val, serializer_type_t& vt, size_t& len ) {

                static std::string_view t_uint8     = typeid(uint8_t).name();
                static std::string_view t_int8      = typeid(int8_t).name();
                static std::string_view t_uint16    = typeid(uint16_t).name();
                static std::string_view t_int16     = typeid(int16_t).name();
                static std::string_view t_uint32    = typeid(uint32_t).name();
                static std::string_view t_int32     = typeid(int32_t).name();
                static std::string_view t_uint64    = typeid(uint64_t).name();
                static std::string_view t_int64     = typeid(int64_t).name();
                static std::string_view t_float     = typeid(float).name();
                static std::string_view t_double    = typeid(double).name();
                static std::string_view t_arr_char  = typeid(serializer_string_t).name();
                static std::string_view t_arr_byte  = typeid(serializer_bin_t).name();
                static std::string_view t_arr_len   = typeid(serializer_len_t).name();
                static std::string_view t_type      = typeid(serializer_type_t).name();

                const std::string_view in_type ( typeid(val).name() );

                vt = serializer_type_t::SERIALIZER_TYPE_UNDEFINED;

                if ( in_type == t_uint8 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_UINT8;
                    len = sizeof (uint8_t);
                } else 
                if ( in_type == t_int8 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_INT8;
                    len = sizeof (int8_t);
                } else 
                if ( in_type == t_uint16 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_UINT16;
                    len = sizeof (uint16_t);
                } else 
                if ( in_type == t_int16 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_INT16;
                    len = sizeof (int16_t);
                } else 
                if ( in_type == t_uint32 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_UINT32;
                    len = sizeof (uint32_t);
                } else 
                if ( in_type == t_int32 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_INT32;
                    len = sizeof (int32_t);
                } else 
                if ( in_type == t_uint64 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_UINT64;
                    len = sizeof (uint64_t);
                } else 
                if ( in_type == t_int64 ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_INT64;
                    len = sizeof (int64_t);
                } else 


                if ( in_type == t_float ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_FLOAT;
                    len = sizeof (float);
                } else 
                if ( in_type == t_double ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_DOUBLE;
                    len = sizeof (double);
                } else


                if ( in_type == t_arr_char ) {
                    vt  = serializer_type_t::SERIALIZER_TYPE_STRING_SIZE_DWORD;
                    len = 0;
                } else
                if ( in_type == t_arr_byte ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_VECTOR_SIZE_DWORD;
                    len = 0;
                } else


                if ( in_type == t_arr_len ) {
                    vt  = serializer_type_t::SERIALIZER_TYPE_ARR_SIZE_DWORD;
                    len = sizeof(uint32_t);
                } else
                if ( in_type == t_type ) {
                    vt  = serializer_type_t::SERIALIZER_TYPE_TYPE;
                    len = sizeof(uint8_t);
                }
            }

        private:
            serializer_state_t      m_state;
            size_t                  m_offset;
            std::stringstream       m_error_msg;
            size_t                  m_error_pos;
    };

}

#endif
