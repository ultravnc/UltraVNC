#pragma once

#include <cstddef>

namespace ViewerBounds {
inline bool range(size_t offset, size_t length, size_t capacity) {
    return offset <= capacity && length <= capacity - offset;
}

inline bool rectangle(int x, int y, int width, int height, int fbWidth, int fbHeight) {
    return x >= 0 && y >= 0 && width >= 0 && height >= 0 &&
        x <= fbWidth && y <= fbHeight && width <= fbWidth - x && height <= fbHeight - y;
}

inline bool pixels(int width, int height, int bytesPerPixel, size_t capacity) {
    return width >= 0 && height >= 0 && bytesPerPixel > 0 && bytesPerPixel <= 4 &&
        (height == 0 || static_cast<size_t>(width) <= capacity / bytesPerPixel / height);
}

inline void clipAxis(int& position, int& length, int limit, int& offset) {
    const long long start = position;
    long long end = start + length;
    const long long clippedStart = start < 0 ? 0 : start;
    if (end > limit) end = limit;
    if (limit <= 0 || length <= 0 || end <= clippedStart) {
        position = length = offset = 0;
        return;
    }
    position = static_cast<int>(clippedStart);
    length = static_cast<int>(end - clippedStart);
    offset = static_cast<int>(clippedStart - start);
}

inline bool payload(unsigned int length) {
    return length <= 104857600u;
}

inline bool framebuffer(int width, int height, int bytesPerPixel) {
    return width > 0 && height > 0 && width <= 20000 && height <= 20000 &&
        pixels(width, height, bytesPerPixel, 0x7fffffff - 256);
}
}
