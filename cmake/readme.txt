######################

# Debian Trixie/unstable amd64 qemu VM 2024-07-28

apt install git curl zip unzip tar build-essential cmake ninja-build mingw-w64 nasm pkg-config ccache



mkdir $HOME/source
cd    $HOME/source
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

./bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=$HOME/source/vcpkg
export PATH=$VCPKG_ROOT:$PATH

vcpkg --version



export PATH=/usr/lib/ccache:$PATH

vcpkg install zlib:x64-mingw-static
vcpkg install zstd:x64-mingw-static
vcpkg install libjpeg-turbo:x64-mingw-static
vcpkg install liblzma:x64-mingw-static
vcpkg install openssl:x64-mingw-static
vcpkg install libsodium:x64-mingw-static



cd    $HOME/source
git clone https://github.com/ultravnc/UltraVNC.git

mkdir obj && cd obj
cmake \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/toolchains/mingw.cmake \
    -DCMAKE_SYSTEM_NAME=MinGW \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
    -DVCPKG_TARGET_ARCHITECTURE=x64 \
    -DVCPKG_APPLOCAL_DEPS=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    ../UltraVNC/cmake
cmake --build . -j
cmake --build . --target install
#make clean

cp -a /usr/lib/gcc/x86_64-w64-mingw32/13-win32/libstdc++-6.dll .
cp -a /usr/lib/gcc/x86_64-w64-mingw32/13-win32/libgcc_s_seh-1.dll .





######################

# Windows with cmake, generate Visual Studio project files

# Install git
# Install Visual Studio Community 2022 (with MFC components to avoid errors about missing afxres.h)

# Open git bash

mkdir c:/source
cd c:/source
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg


# x64 Native Tools Command Prompt for VS 2022

cd /d c:\source\vcpkg
bootstrap-vcpkg.bat -disableMetrics
set VCPKG_ROOT=c:\source\vcpkg
set PATH=%VCPKG_ROOT%;%PATH%

vcpkg --version


# If you get this message:
#   vcpkg could not locate a manifest (vcpkg.json);
#   https://superuser.com/questions/1829880/vcpkg-could-not-locate-a-manifest-vcpkg-json
doskey vcpkg=

vcpkg install zlib:x64-windows-static
vcpkg install zstd:x64-windows-static
vcpkg install libjpeg-turbo:x64-windows-static
vcpkg install liblzma:x64-windows-static
vcpkg install openssl:x64-windows-static
vcpkg install libsodium:x64-windows-static

vcpkg integrate install


cd /d c:\source
git clone https://github.com/ultravnc/UltraVNC.git

mkdir obj && cd obj
cmake ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    ..\UltraVNC\cmake
set CL=/MP
cmake --build . --parallel --config=RelWithDebInfo




######################

# Windows with cmake, using ninja build system, and address santitizer enabled

# x64 Native Tools Command Prompt for VS 2022

# Same steps before the cmake invocation as above

cd /d c:\source
mkdir obj_ninja && cd obj_ninja
cmake ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    -G Ninja ^
    -Dasan=TRUE ^
    ..\UltraVNC\cmake
cmake --build . --parallel --config=RelWithDebInfo
cmake --build . --target install --config=RelWithDebInfo
copy "%VCToolsInstallDir%\bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll" ultravnc\
#ninja clean




######################

# Windows with cmake, using LLVM compiler

# x64 Native Tools Command Prompt for VS 2022

# Same steps before the cmake invocation as above

cd /d c:\source
mkdir obj_ninja_llvm && cd obj_ninja_llvm
cmake ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    -T ClangCL ^
    ..\UltraVNC\cmake
cmake --build . --parallel --config=RelWithDebInfo
#ninja clean




######################

# Windows with regular Visual Studio project files

# Install Plattformtoolset matching to the project files, currently v142

set _P=^
  /p:Platform=x64 ^
  /p:Configuration=Release ^
  /p:Plattformtoolset=v143 ^
  /p:BuildInParallel=true -maxcpucount:16 /p:CL_MPCount=16 ^
  /t:Clean;Build
set CL=/MP

cd /d C:\source\UltraVNC
msbuild %_P% winvnc\winvnc.sln
msbuild %_P% vncviewer\vncviewer.sln
