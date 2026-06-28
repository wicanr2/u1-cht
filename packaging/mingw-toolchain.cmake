# MinGW-w64 交叉編譯工具鏈(Linux → Windows x86_64)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# SDL2 sysroot(由 build-windows.sh 以 -DMINGW_SDL_PREFIX 傳入)
if(DEFINED MINGW_SDL_PREFIX)
    list(APPEND CMAKE_FIND_ROOT_PATH ${MINGW_SDL_PREFIX})
    set(ENV{PKG_CONFIG_LIBDIR} "${MINGW_SDL_PREFIX}/lib/pkgconfig")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 靜態連 libgcc/libstdc++(不用 -static,否則找不到 SDL 的 import lib);
# winpthread 仍是 DLL 依賴,由 build-windows.sh 一併打包。
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
