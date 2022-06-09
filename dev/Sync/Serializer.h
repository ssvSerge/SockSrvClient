#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <stdint.h>

#include <vector>
#include <string>

#define UNREF(x)   (void)(x)


namespace hid {

    // Reader and Writer must be synchronized with the length of variables
    // Assume we're on the 64 bit CPU archtecture.
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

    typedef std::string          serializer_string_t;
    typedef std::vector<uint8_t> serializer_bin_t;
    typedef std::vector<uint8_t> serializer_storage_t;

    enum class serializer_state_t {
        SERIALIZER_STATE_UNKNOWN,
        SERIALIZER_STATE_OK,
        SERIALIZER_STATE_FAILED
    };

    enum class serializer_len_t: uint32_t {
        SERIALIZER_LEN_ZERO
    };

    enum class serializer_type_t : uint8_t {
        SERIALIZER_TYPE_UNDEFINED   = 0,
        SERIALIZER_TYPE_INT8        = 0x11,
        SERIALIZER_TYPE_UINT8       = 0x12,
        SERIALIZER_TYPE_INT16       = 0x13,
        SERIALIZER_TYPE_UINT16      = 0x14,
        SERIALIZER_TYPE_INT32       = 0x15,
        SERIALIZER_TYPE_UINT32      = 0x16,
        SERIALIZER_TYPE_INT64       = 0x17,
        SERIALIZER_TYPE_UINT64      = 0x18,
        SERIALIZER_TYPE_FLOAT       = 0x19,
        SERIALIZER_TYPE_DOUBLE      = 0x1A,
        SERIALIZER_TYPE_VAR_CHAR    = 0x21,
        SERIALIZER_TYPE_VAR_BYTE    = 0x22,
        SERIALIZER_TYPE_ARR         = 0x30,
        SERIALIZER_TYPE_TYPE        = 0x40,
        SERIALIZER_TYPE_LASTID
    };

    class Serializer {

        public:
            void Reset () {
                m_state  = serializer_state_t::SERIALIZER_STATE_OK;
                m_offset = 0;
            }

        public:
            template<typename T>
            void StoreFix ( const T& var, serializer_storage_t& storage ) {

                serializer_type_t vt;
                serializer_len_t  vl;

                if ( m_state == serializer_state_t::SERIALIZER_STATE_OK ) {

                    try {

                        get_type(var, vt, vl);

                        switch( vt ) {
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
                                // TRACE_ERROR ("Unknown variable type");
                                m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                                return;
                        }

                        store_type ( vt, storage );
                        store_dat  ( &var, vl, storage );

                    } catch ( ... ) {
                        // TRACE_ERROR ("Exception at StoreFix");
                        m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }
                }

            }

            template<typename T>
            void StoreVar ( const T& val, serializer_storage_t& storage ) {

                serializer_type_t   vt;  // value type
                serializer_len_t    vl;  // value length
                const uint8_t*      lp = static_cast<uint8_t*> ( &vl );

                if ( m_state == serializer_state_t::SERIALIZER_STATE_OK ) {

                    try {

                        get_type ( val, vt );

                        switch ( vt ) {
                            case serializer_type_t::SERIALIZER_TYPE_VAR_CHAR:
                                vl = static_cast<serializer_len_t> (val.size ());
                                break;
                            case serializer_type_t::SERIALIZER_TYPE_VAR_BYTE:
                                vl = static_cast<serializer_len_t> (val.length ());
                                break;
                            default:
                                // TRACE_ERROR ("Unknown variable type");
                                m_state == serializer_state_t::SERIALIZER_STATE_FAILED;
                                return;
                        }

                        store_type ( vt, storage );
                        store_len  ( vl, storage );
                        store_dat  ( val.data(), vl, storage );

                    } catch( ... ) {
                        // TRACE_ERROR ("Exception at StoreVar");
                        m_state == serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                }
            }

            void StoreArr ( uint32_t cnt, serializer_storage_t& storage ) {

                serializer_type_t vt = serializer_type_t::SERIALIZER_TYPE_ARR;

                if ( m_state == serializer_state_t::SERIALIZER_STATE_OK ) {
                    store_type ( vt,  storage );
                    store_len  ( cnt, storage );
                }

            }


        public:
            template<typename T>
            void LoadFix ( serializer_storage_t& storage, const T& val ) {

                if ( m_state == serializer_state_t::SERIALIZER_STATE_OK ) {

                    serializer_type_t   vt;
                    serializer_type_t   st;
                    serializer_len_t vl;

                    get_type ( val, vt );

                    switch ( vt ) {
                        case serializer_type_t::SERIALIZER_TYPE_INT8:
                        case serializer_type_t::SERIALIZER_TYPE_UINT8:
                            vl = 1;
                            break;
                        case serializer_type_t::SERIALIZER_TYPE_INT16:
                        case serializer_type_t::SERIALIZER_TYPE_UINT16:
                            vl = 2;
                            break;
                        case serializer_type_t::SERIALIZER_TYPE_INT32:
                        case serializer_type_t::SERIALIZER_TYPE_UINT32:
                            vl = 4;
                            break;
                        case serializer_type_t::SERIALIZER_TYPE_INT64:
                        case serializer_type_t::SERIALIZER_TYPE_UINT64:
                            vl = 8;
                            break;
                        default:
                            // TRACE_ERROR ("Unknown variable type");
                            m_state == serializer_state_t::SERIALIZER_STATE_FAILED;
                            return;
                    }

                    load_type ( storage, st );

                    if ( vt != st ) {
                        // TRACE_ERROR ("Wrong type. Expected: %d; Received: %d", vt, st);
                        m_state == serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                    load_dat ( storage, &val, vl );

                }
            }

            template<typename T>
            void LoadVar ( serializer_storage_t& storage, const T& val ) {

                if ( m_state == serializer_state_t::SERIALIZER_STATE_OK ) {

                    serializer_type_t   vt;
                    serializer_type_t   st;
                    serializer_len_t    vl;

                    get_type ( val, vt );

                    switch ( vt ) {
                        case serializer_type_t::SERIALIZER_TYPE_VAR_CHAR:
                        case serializer_type_t::SERIALIZER_TYPE_VAR_BYTE:
                            break;
                        default:
                            // TRACE_ERROR ("Unknown variable type");
                            m_state == serializer_state_t::SERIALIZER_STATE_FAILED;
                            return;
                    }

                    load_type ( storage, st );

                    if ( vt != st ) {
                        // TRACE_ERROR ("Wrong type. Expected: %d; Received: %d", vt, st);
                        m_state == serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                    load_len ( storage, &val, vl );
                    load_dat ( storage, val.data(), vl );
                }

            }

            void LoadArr ( serializer_storage_t& storage, serializer_len_t& cnt ) {

                if ( m_state == serializer_state_t::SERIALIZER_STATE_OK ) {

                    serializer_type_t   st;

                    load_type ( storage, st );

                    if ( serializer_type_t::SERIALIZER_TYPE_ARR != st ) {
                        // TRACE_ERROR ("Wrong type. Expected: %d; Received: %d", vt, st);
                        m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                    }

                    load_len ( storage, cnt );

                }
            }

        private:
            void store_type ( serializer_type_t& val_type, serializer_storage_t& storage ) {
                try {
                    check_size (storage, 1);
                    storage[m_offset] = static_cast<uint8_t> (val_type);
                    m_offset += sizeof ( uint8_t );
                } catch( ... ) {
                    // TRACE_ERROR ( "Exception" );
                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                }
            }

            void store_len  ( uint32_t param, serializer_storage_t& storage ) {
                try {
                    const uint8_t* const bin = reinterpret_cast<const uint8_t*> (&param);
                    check_size (storage, sizeof(param) );
                    memcpy ( storage.data()+m_offset, bin, sizeof(param) );
                    m_offset += sizeof(param);
                } catch( ... ) {
                    // TRACE_ERROR ( "Exception" );
                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                }
            }

            void store_dat  ( const void* const param, serializer_len_t len, serializer_storage_t& storage ) {
                try {
                    const uint8_t* const bin = static_cast<const uint8_t*> (param);
                    const uint32_t       cnt = static_cast<uint32_t> (len);
                    check_size (storage, cnt);
                    memcpy ( storage.data()+m_offset, bin, cnt);
                    m_offset += cnt;
                } catch( ... ) {
                    // TRACE_ERROR ( "Exception" );
                    m_state = serializer_state_t::SERIALIZER_STATE_FAILED;
                }
            }

        private:
            void load_type  ( serializer_storage_t& storage, serializer_type_t& val_type ) {
                UNREF (storage);
                UNREF (val_type);
            }

            void load_len   ( serializer_storage_t& storage, serializer_len_t& val_len ) {
                UNREF (storage);
                UNREF (val_len);
            }

            void load_dat   ( serializer_storage_t& storage, const void* const val, serializer_len_t len, serializer_type_t& val_type ) {
                UNREF (storage);
                UNREF (len);
                UNREF (val);
                UNREF (val_type);
            }

        private:
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

        private:
            template<typename T>
            static void get_type ( const T val, serializer_type_t& vt, serializer_len_t& vl ) {

                static std::string_view t_uint8     (typeid(uint8_t).name()  );
                static std::string_view t_int8      (typeid(int8_t).name()   );
                static std::string_view t_uint16    (typeid(uint16_t).name() );
                static std::string_view t_int16     (typeid(int16_t).name()  );
                static std::string_view t_uint32    (typeid(uint32_t).name() );
                static std::string_view t_int32     (typeid(int32_t).name()  );
                static std::string_view t_uint64    (typeid(uint64_t).name() );
                static std::string_view t_int64     (typeid(int64_t).name()  );
                static std::string_view t_float     (typeid(float).name()    );
                static std::string_view t_double    (typeid(double).name()   );
                static std::string_view t_arr_char  (typeid(serializer_string_t).name() );
                static std::string_view t_arr_byte  (typeid(serializer_bin_t).name() );
                static std::string_view t_arr_len   (typeid(serializer_len_t).name() );
                static std::string_view t_type      (typeid(serializer_type_t).name() );

                uint32_t len = 0;
                std::string_view in_type ( typeid(val).name() );

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
                    vt = serializer_type_t::SERIALIZER_TYPE_VAR_CHAR;
                } else
                if ( in_type == t_arr_byte ) {
                    vt = serializer_type_t::SERIALIZER_TYPE_VAR_BYTE;
                } else


                if ( in_type == t_arr_len ) {
                    vt  = serializer_type_t::SERIALIZER_TYPE_ARR;
                    len = sizeof(uint32_t);
                } else
                if ( in_type == t_type ) {
                    vt  = serializer_type_t::SERIALIZER_TYPE_TYPE;
                    len = sizeof(uint8_t);
                }

                vl = static_cast<serializer_len_t> (len);

            }

        private:
            serializer_state_t  m_state;
            size_t              m_offset;

    };


}

#endif
