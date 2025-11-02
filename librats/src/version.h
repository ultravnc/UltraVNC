#pragma once

#include "rats_export.h"

// Version information for librats
#define LIBRATS_VERSION_MAJOR 1
#define LIBRATS_VERSION_MINOR 0
#define LIBRATS_VERSION_PATCH 0
#define LIBRATS_VERSION_BUILD 0
#define LIBRATS_VERSION_STRING "1.0.0"
#define LIBRATS_GIT_DESCRIBE "v1.0.0"

// ASCII Art Header
#define LIBRATS_ASCII_HEADER \
"        #######    #####   ########  #######       \n" \
"        ##    ##  ##   ##     ##     ##            \n" \
"        ##    ##  ##   ##     ##     ##            \n" \
"        #######   #######     ##     #######       \n" \
"        ##  ##    ##   ##     ##           ##      \n" \
"        ##   ##   ##   ##     ##           ##      \n" \
"        ##    ##  ##   ##     ##     #######       \n" \
"                                                    \n" \
"           P2P Network Communication Library       \n" \
"        ======================================      \n"

namespace librats {
    namespace version {
        // Version information
        const int MAJOR = LIBRATS_VERSION_MAJOR;
        const int MINOR = LIBRATS_VERSION_MINOR;
        const int PATCH = LIBRATS_VERSION_PATCH;
        const int BUILD = LIBRATS_VERSION_BUILD;
        const char* const STRING = LIBRATS_VERSION_STRING;
        const char* const GIT_DESCRIBE = LIBRATS_GIT_DESCRIBE;
        
        // ASCII header
        const char* const ASCII_HEADER = LIBRATS_ASCII_HEADER;
        
        // Print version info
        RATS_API void rats_print_version_info();
        
        // Print ASCII header with version
        RATS_API void rats_print_header();
    }
}
