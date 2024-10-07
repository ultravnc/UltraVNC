zlib:x64-windows-static                           1.3                 A compression library
zlib:x86-windows                                  1.3                 A compression library
zlib:x86-windows-static                           1.3                 A compression library
zlib:x64-windows
zstd:x64-windows-static                           1.5.5#1             Zstandard - Fast real-time compression algorithm
zstd:x86-windows                                  1.5.5#1             Zstandard - Fast real-time compression algorithm
zstd:x86-windows-static                           1.5.5#1             Zstandard - Fast real-time compression algorithm
zstd:x64-windows 
libjpeg-turbo:x64-windows-static                  3.0.0#1             libjpeg-turbo is a JPEG image codec that uses SI...
libjpeg-turbo:x86-windows                         3.0.0#1             libjpeg-turbo is a JPEG image codec that uses SI...
libjpeg-turbo:x86-windows-static                  3.0.0#1             libjpeg-turbo is a JPEG image codec that uses SI...
libjpeg-turbo:x64-windows  
liblzma:x64-windows-static                        5.4.3#1             Compression library with an API similar to that ...
liblzma:x86-windows                               5.4.3#1             Compression library with an API similar to that ...
liblzma:x86-windows-static                        5.4.3#1             Compression library with an API similar to that ...
liblzma:x64-windows 

libsodium:x86-windows-static
libsodium:x86-windows
libsodium:x64-windows-static
libsodium:x64-windows

vcpkg install zlib:x86-windows
vcpkg install zstd:x86-windows-static
..
vcpkg integrate install
