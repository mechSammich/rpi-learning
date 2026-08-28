# toolchain-rpi.cmake
# CMake toolchain file for cross-compiling to Raspberry Pi Zero 2 W
# (ARMv7 hard-float ABI) from an x86-64 Linux host.
#
# Prerequisites on the host:
#   sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7)

# Cross-compiler executables.
set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Tune for the Cortex-A53 used in the RP3A0 SoC (RPi Zero 2 W).
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard")

# Sysroot (optional).  Set RPI_SYSROOT in your environment or on the cmake
# command line if you have an RPi OS sysroot available:
#   cmake ... -DRPI_SYSROOT=/path/to/sysroot
if(DEFINED RPI_SYSROOT)
    set(CMAKE_SYSROOT "${RPI_SYSROOT}")
    set(CMAKE_FIND_ROOT_PATH "${RPI_SYSROOT}")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
