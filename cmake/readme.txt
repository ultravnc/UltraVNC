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

# Common Steps cmake invocation

# Install git
# Install Visual Studio Community 2022 (with MFC components to avoid errors about missing afxres.h)
#	If you have Visual Studio Build Tools 2019 with VSC 2022 (install on it MFC components too for same reason)
#		and install Windows SDK Version 8.1 (for Errors errors MSB8036)

# Open git bash

mkdir c:/source
cd c:/source
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
ls
#^ For check install and if the file bootstrap-vcpkg.bat is here.


# Developer Command Prompt for VS
# (/!\ Don't Open Visual Studio until you have configure all, may be the environment crash your install! So open the prompt from explorer i.e. "Windows Studio 2022" the folder -and-> "Developer Command Prompt for VS 2022" shortcut)

cd /d c:\source\vcpkg
bootstrap-vcpkg.bat -disableMetrics

After the batch command

On command line
---------------
set VCPKG_ROOT=c:\source\vcpkg
set PATH=%VCPKG_ROOT%;%PATH%

OR set on Windows environment variables
---------------------------------------
Create System environment variables :
Name  : VCPKG_ROOT
Value : C:\source\vcpkg
Adding to the System Path
c:\source\vcpkg
%VCPKG_ROOT%

# If you have error ZLIB not find
On command line
---------------
set VCPKG_DEFAULT_TRIPLET=x64-windows-static

OR set on Windows environment variables
---------------------------------------
Create System user variables :
Name  : VCPKG_DEFAULT_TRIPLET
Value : x64-windows-static
(Nota : previously I test with these in Windows System environment variables)

After If error Zlib still active try these
On Visual Studio 2022 and open CMakeSettings.json
Change these line
"cmakeToolchain": "%VCPKG_ROOT%\\scripts\\buildsystems\\vcpkg.cmake"
with
"cmakeToolchain": "C:\\source\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake"
KO ne fonctionne pas.

# See https://stackoverflow.com/questions/55496611/cmake-cannot-find-libraries-installed-with-vcpkg
# Reinstall all vcpkg if you have install them before

vcpkg --version


# If you get this message:
#   vcpkg could not locate a manifest (vcpkg.json) (when you try to install zlib below);
#   https://superuser.com/questions/1829880/vcpkg-could-not-locate-a-manifest-vcpkg-json
doskey vcpkg=
# End vcpkg could not locate a manifest 

# If you have an error ZLIB not find don't do these see (# If you have error ZLIB not find) and below
vcpkg install zlib:x64-windows-static
vcpkg install zstd:x64-windows-static
vcpkg install libjpeg-turbo:x64-windows-static
vcpkg install liblzma:x64-windows-static
vcpkg install openssl:x64-windows-static

# Patch error ZLIB not find doing these see (# If you have error ZLIB not find)
vcpkg install zlib
vcpkg install zstd
vcpkg install libjpeg-turbo
vcpkg install liblzma
vcpkg install openssl

vcpkg integrate install


cd /d c:\source
git clone https://github.com/ultravnc/UltraVNC.git

# End Common Steps cmake invocation


# Steps specific using cmake, generate Visual Studio project files
cd /d c:\source
mkdir obj && cd obj
cmake ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    ..\UltraVNC\cmake
set CL=/MP
cmake --build . --parallel --config=RelWithDebInfo




######################

# Windows with cmake, using ninja build system, and address sanitizer enabled

# x64 Native Tools Command Prompt for VS 2022

# Same steps before the cmake invocation as above (# Common Steps cmake invocation)

# Steps specific using ninja build system, and address sanitizer enabled
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

# Same steps before the cmake invocation as above (# Common Steps cmake invocation)

# Steps specific using LLVM compiler
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

# Windows with regular Visual Studio project files (with MFC components to avoid errors about missing afxres.h)
#	If you have Visual Studio Build Tools 2019 with VSC 2022 (install on it MFC components too for same reason)
#		Download and install Windows SDK Version 8.1 (for Errors MSB8036)
#		From VS 2022 installer and install "MSVC v140 - VS 2015 C++ Build Tools (v14.00)" (for fatal error C1083: Unable to open include file : 'ctype.h')

# Same steps before the cmake invocation as above (# Common Steps cmake invocation)

# Install PlatformToolset matching to the project files, currently v143

set _P=^
  /p:Platform=x64 ^
  /p:Configuration=Release ^
  /p:PlatformToolset=v143 ^
  /p:BuildInParallel=true -maxcpucount:16 /p:CL_MPCount=16 ^
  /t:Clean;Build
set CL=/MP

cd /d C:\source\UltraVNC
msbuild %_P% winvnc\winvnc.sln
msbuild %_P% vncviewer\vncviewer.sln




######################

# Windows with cmake, generate Qt project files (Qt Community Tests Specific)

# Install git
# Install Visual Studio Community 2022 (with MFC components to avoid errors about missing afxres.h)
# Install Qt Creator Edition Community

# Cross-platform Qt 6 CMake Project Setup (Windows)
# System Variables and Path of Qt in Environnements Variables

New Variable (if doesn't exist)
Variable name		: QTDIR
Variable value		: C:\Qt\6.7.2\msvc2019_64 (for msvc2019_64 or the path you put the compiler you want to use on your computer)

Edit Path, add bin and lib
# New (if doesn't exist)
%QTDIR%\bin
%QTDIR%\lib

# New (Patch Error ZLIB not find see (Section: # Windows with cmake, generate Visual Studio project files))
Variable name	: VCPKG_DEFAULT_TRIPLET
Variable value	: x64-windows-static

# Uninstall the vcpkg optional package from your Visual Studio instance (see VCPKG_README.txt if you want knowing why).
# Edit Windows system environment variable
# New (if doesn't exist)
Variable name	: VCPKG_ROOT
Variable value	: C:\source\vcpkg

# Adding to Path
%VCPKG_ROOT%

Restart your computer.

# Same steps before the cmake invocation as above (Section: # Windows with cmake, generate Visual Studio project files)

# Install cmake (Windows)
# Download and install from https://cmake.org/download/
# Windows x64 Installer: cmake-3.30.2-windows-x86_64.msi (or version more recent and for your computer OS Specific)

# On CMake (cmake-gui) use these
	Where is the the source code 	: C:/source/UltraVNC/cmake
	Where to build the binaries 	: C:/source/UltraVNC/build
	Click on Configure for choose compiler version (Don't miss clic you can't go back to choose another).
	(If Error ZLIB not find : try to update environment and see VCPKG_README.txt doesn't work for me actually. Note: same error with Visual Studio 2022)

# Steps specific Qt Creator Community Edition below (In Progress)