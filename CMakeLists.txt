# Ultima I 中文化 — Linux/SDL2 建置設定
# 由上游 open_ultima (vcpkg/MSVC) 改寫為 Ubuntu apt + pkg-config。
# 上游相依 taoJSON → 改用 nlohmann/json (apt: nlohmann-json3-dev)。
cmake_minimum_required(VERSION 3.17)
project(u1_cht CXX C)   # C 用於 vendored SDL2_gfx

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

# nlohmann/json:有系統套件用之,否則用 vendored 單檔(MinGW 交叉編譯無 CMake package)
find_package(nlohmann_json 3 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json: using vendored third_party/nlohmann/json.hpp")
    add_library(u1_nlohmann_json INTERFACE)
    add_library(nlohmann_json::nlohmann_json ALIAS u1_nlohmann_json)
    target_include_directories(u1_nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
endif()

# SDL2_gfx 無官方跨平台二進位 → vendor 原始碼直接編進(Windows/macOS/Linux 一致,免系統依賴)
set(SDL2_GFX_SRC
        third_party/SDL2_gfx/SDL2_gfxPrimitives.c
        third_party/SDL2_gfx/SDL2_rotozoom.c)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(u1_cht main.cpp ${SOURCES} ${SDL2_GFX_SRC})

target_include_directories(u1_cht PRIVATE
        ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS}
        third_party/SDL2_gfx
        src)

# pkg-config 給的是 library 名稱;link dir 要另外加(交叉編譯 sysroot 不在預設搜尋路徑)
target_link_directories(u1_cht PRIVATE
        ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS}
        ${SDL2_TTF_LIBRARY_DIRS} ${SDL2_MIXER_LIBRARY_DIRS})

target_link_libraries(u1_cht PRIVATE
        ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES} ${SDL2_MIXER_LIBRARIES}
        nlohmann_json::nlohmann_json)

# 資源(含 CJK 字型與字串表)複製到執行檔旁
file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
