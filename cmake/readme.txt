
# cmake, mingw-w64 and nasm should be installed

git clone https://github.com/bernhardu/UltraVNC.git
mkdir obj
cd obj
cmake --toolchain ../UltraVNC/cmake/toolchain-mingw32-w64_x86_64.cmake ../UltraVNC/cmake
make -j`nproc`
