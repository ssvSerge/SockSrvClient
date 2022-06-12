#ifndef __HIDTYPES_H__
#define __HIDTYPES_H__

#include <vector>

namespace hid {

    typedef std::string             serializer_string_t;
    typedef std::vector<uint8_t>    serializer_bin_t;
    typedef std::vector<uint8_t>    serializer_storage_t;

}

#endif
