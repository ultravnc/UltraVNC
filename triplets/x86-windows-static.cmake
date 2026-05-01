set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# Only minizip-ng needs Win7 targeting to avoid CreateFile2 (Win8+ only)
if(PORT MATCHES "minizip-ng")
    set(VCPKG_C_FLAGS "/D_WIN32_WINNT=0x0601 /DWINVER=0x0601")
    set(VCPKG_CXX_FLAGS "/D_WIN32_WINNT=0x0601 /DWINVER=0x0601")
endif()
