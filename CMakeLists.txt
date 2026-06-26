# Ultima I 中文化 — Linux/SDL2 建置設定
# 由上游 open_ultima (vcpkg/MSVC) 改寫為 Ubuntu apt + pkg-config。
# 上游相依 taoJSON → 改用 nlohmann/json (apt: nlohmann-json3-dev)。
cmake_minimum_required(VERSION 3.17)
project(u1_cht CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_GFX REQUIRED SDL2_gfx)
find_package(nlohmann_json 3 REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(u1_cht main.cpp ${SOURCES})

target_include_directories(u1_cht PRIVATE
        ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_GFX_INCLUDE_DIRS}
        src)

target_link_libraries(u1_cht PRIVATE
        ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES} ${SDL2_GFX_LIBRARIES}
        nlohmann_json::nlohmann_json)

# 資源(含 CJK 字型與字串表)複製到執行檔旁
file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
