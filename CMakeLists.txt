cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg toolchain — must be before project()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "")

project(StardewNotionWidget LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(StardewNotionWidget
    src/main.cpp
    src/overlay/window.cpp
    src/cache/CacheStore.cpp
)


target_include_directories(StardewNotionWidget PRIVATE include)

target_link_libraries(StardewNotionWidget PRIVATE
    nlohmann_json::nlohmann_json
    httplib::httplib
    SQLiteCpp
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32
    gdi32
    user32
)

target_compile_definitions(StardewNotionWidget PRIVATE
    UNICODE
    _UNICODE
    CPPHTTPLIB_OPENSSL_SUPPORT
    _WIN32_WINNT=0x0A00
    NTDDI_VERSION=0x0A000000
)

set_target_properties(StardewNotionWidget PROPERTIES OUTPUT_NAME "StardewNotionWidget")