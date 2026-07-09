set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX "C:/Users/gollu/Documents/AntiGravity/2_Demake_Slug/Trabalho-Final-DE1-S0C---Sistema-Embarcados-2026.1/build/arm-gcc/gcc-arm")
set(CMAKE_SYSROOT "${TOOLCHAIN_PREFIX}/arm-none-linux-gnueabihf/libc")

set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}/bin/arm-none-linux-gnueabihf-gcc.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}/bin/arm-none-linux-gnueabihf-g++.exe")

set(CMAKE_C_FLAGS "--sysroot=${CMAKE_SYSROOT} -isystem ${CMAKE_SYSROOT}/usr/include -isystem ${CMAKE_SYSROOT}/usr/include/arm-linux-gnueabihf")
set(CMAKE_CXX_FLAGS "--sysroot=${CMAKE_SYSROOT} -isystem ${CMAKE_SYSROOT}/usr/include -isystem ${CMAKE_SYSROOT}/usr/include/arm-linux-gnueabihf")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
