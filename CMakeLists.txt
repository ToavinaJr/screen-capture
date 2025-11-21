cmake_minimum_required(VERSION 3.15)
project(MultimediaStreamingApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_SERVER "Build server component" ON)
option(BUILD_CLIENT "Build client component" ON)
option(ENABLE_TLS "Enable TLS support" ON)
option(ENABLE_AUDIO "Enable audio capture" ON)

# vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    message(STATUS "Using vcpkg toolchain: $ENV{VCPKG_ROOT}")
endif()

# Support Windows
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
endif()

# Find SDL2
find_package(SDL2 CONFIG REQUIRED)
message(STATUS "SDL2 found: ${SDL2_INCLUDE_DIRS}")

# Find OpenSSL (optional)
if(ENABLE_TLS)
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
        add_definitions(-DENABLE_TLS)
    else()
        message(WARNING "OpenSSL not found, TLS disabled")
        set(ENABLE_TLS OFF)
    endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Common sources
set(COMMON_SOURCES
    src/utils/Logger.cpp
    src/threading/ThreadPool.cpp
    src/capture/ScreenCapture.cpp
)

# Main application executable
add_executable(screen_share
    src/main.cpp
    src/core/Application.cpp
    src/display/SDLRenderer.cpp
    src/network/StreamServer.cpp
    src/audio/MicrophoneCapture.cpp
    ${COMMON_SOURCES}
)

# Link SDL2
target_link_libraries(screen_share PRIVATE SDL2::SDL2 SDL2::SDL2main)

# Link X11 for Linux screen capture
if(NOT WIN32)
    find_package(X11 REQUIRED)
    target_link_libraries(screen_share PRIVATE ${X11_LIBRARIES})
    target_include_directories(screen_share PRIVATE ${X11_INCLUDE_DIR})
endif()

# Link OpenSSL if enabled
if(ENABLE_TLS)
    target_link_libraries(screen_share PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_sources(screen_share PRIVATE src/network/TLSConnection.cpp)
endif()

# Link Windows libraries
if(WIN32)
    target_link_libraries(screen_share PRIVATE ws2_32)
else()
    target_link_libraries(screen_share PRIVATE pthread)
endif()

# Tests (optional)
option(BUILD_TESTS "Build test executables" OFF)

if(BUILD_TESTS)
    add_executable(test_logger
        src/utils/Logger.cpp
        tests/test_logger_minimal.cpp
    )

    add_executable(test_threadpool
        src/threading/ThreadPool.cpp
        tests/test_threading.cpp
    )

    add_executable(test_common
        tests/test_common.cpp
    )
    
    add_executable(test_microphone
        src/audio/MicrophoneCapture.cpp
        src/utils/Logger.cpp
        tests/test_microphone.cpp
    )
    target_link_libraries(test_microphone PRIVATE SDL2::SDL2 SDL2::SDL2main)
    if(WIN32)
        target_link_libraries(test_microphone PRIVATE ws2_32)
    else()
        target_link_libraries(test_microphone PRIVATE pthread)
    endif()
    
    add_executable(test_stream_server
        src/network/StreamServer.cpp
        src/utils/Logger.cpp
        tests/test_stream_server.cpp
    )
    if(WIN32)
        target_link_libraries(test_stream_server PRIVATE ws2_32)
    else()
        target_link_libraries(test_stream_server PRIVATE pthread)
    endif()
    
    add_executable(test_e2e_streaming
        src/network/StreamServer.cpp
        src/network/StreamClient.cpp
        src/utils/Logger.cpp
        tests/test_e2e_streaming.cpp
    )
    if(WIN32)
        target_link_libraries(test_e2e_streaming PRIVATE ws2_32)
    else()
        target_link_libraries(test_e2e_streaming PRIVATE pthread)
    endif()
    
    add_executable(test_viewer
        src/network/StreamClient.cpp
        src/utils/Logger.cpp
        tests/test_viewer.cpp
    )
    if(WIN32)
        target_link_libraries(test_viewer PRIVATE ws2_32)
    else()
        target_link_libraries(test_viewer PRIVATE pthread)
    endif()
    
    add_executable(test_stream_app
        src/network/StreamServer.cpp
        src/utils/Logger.cpp
        tests/test_stream_app.cpp
    )
    if(WIN32)
        target_link_libraries(test_stream_app PRIVATE ws2_32)
    else()
        target_link_libraries(test_stream_app PRIVATE pthread)
    endif()
    
    add_executable(test_visual_viewer
        src/network/StreamClient.cpp
        src/utils/Logger.cpp
        tests/test_visual_viewer.cpp
    )
    target_link_libraries(test_visual_viewer PRIVATE SDL2::SDL2 SDL2::SDL2main)
    if(WIN32)
        target_link_libraries(test_visual_viewer PRIVATE ws2_32)
    else()
        target_link_libraries(test_visual_viewer PRIVATE pthread)
    endif()
endif()

message(STATUS "===================================")
message(STATUS "Multimedia Streaming App Configuration")
message(STATUS "Build Server: ${BUILD_SERVER}")
message(STATUS "Build Client: ${BUILD_CLIENT}")
message(STATUS "TLS Support: ${ENABLE_TLS}")
message(STATUS "Audio Support: ${ENABLE_AUDIO}")
message(STATUS "===================================")
