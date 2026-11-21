#pragma once

#include <cstdio>
#ifdef _VCPKG
#include <jpeglib.h>
#else
#include "libjpeg-turbo-win/jpeglib.h"
#endif

namespace ViewerJpeg {
inline void initSource(j_decompress_ptr cinfo) {
    *static_cast<bool*>(cinfo->client_data) = false;
}

inline boolean fillInputBuffer(j_decompress_ptr cinfo) {
    static const JOCTET endMarker[] = {0xff, JPEG_EOI};
    *static_cast<bool*>(cinfo->client_data) = true;
    cinfo->src->bytes_in_buffer = sizeof(endMarker);
    cinfo->src->next_input_byte = endMarker;
    return TRUE;
}

inline void skipInputData(j_decompress_ptr cinfo, long bytes) {
    if (bytes <= 0) return;
    if (static_cast<size_t>(bytes) > cinfo->src->bytes_in_buffer) {
        fillInputBuffer(cinfo);
    } else {
        cinfo->src->next_input_byte += bytes;
        cinfo->src->bytes_in_buffer -= bytes;
    }
}
}
