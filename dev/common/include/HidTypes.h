#ifndef __HIDTYPES_H__
#define __HIDTYPES_H__

#include <vector>
#include <string>

namespace hid {

namespace types {

    #ifndef UNUSED
    #define UNUSED(x)                  (void) (x);
    #endif

    #define IN
    #define OUT
    #define INOUT
    #define MANDATORY
    #define OPTIONAL

    typedef std::string                string_t;
    typedef std::vector<uint8_t>       storage_t;

}

}

#endif
