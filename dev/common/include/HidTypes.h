#ifndef __HIDTYPES_H__
#define __HIDTYPES_H__

#include <vector>
#include <string>

namespace hid {

namespace types {

    // typedef std::string             serializer_string_t;

    typedef std::string                string_t;
    typedef std::vector<uint8_t>       storage_t;

    // typedef std::vector<uint8_t>    serializer_bin_t;
    // typedef std::vector<uint8_t>    serializer_storage_t;

}

}

#endif
