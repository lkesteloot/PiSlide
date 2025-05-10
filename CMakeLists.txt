
# sudo apt install -y cmake libssl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libsqlite3-dev

# Configure, build, and run:
# cmake -S . -B build && cmake --build build --target run --parallel

# Need 3.16 for good C++23 support.
cmake_minimum_required(VERSION 3.16)

# Fetch and build cpr (HTTP library).
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 1.11.2) # https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

# Fetch and build nlohmann JSON.
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# Fetch and build raylib.
FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git
                            GIT_TAG 5.5) # https://github.com/raysan5/raylib/releases
set(BUILD_EXAMPLES OFF)
set(PLATFORM Desktop)
set(GRAPHICS GRAPHICS_API_OPENGL_21)
FetchContent_MakeAvailable(raylib)

# Name of the project, using C++.
project(PiSlide LANGUAGES CXX)

# Require C++23, no fancy GCC extensions.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find all source files.
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Define our program.
add_executable(pislide ${SOURCES})

# Strict compile options.
target_compile_options(pislide PRIVATE -Wall -Werror -g)

# Libraries we need.
target_link_libraries(pislide PRIVATE raylib sqlite3 cpr::cpr nlohmann_json::nlohmann_json raylib)
if(APPLE)
    target_link_libraries(pislide PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework CoreVideo"
        "-framework AudioToolbox"
    )
endif()

# Custom run target. Need X Windows on Linux.
if(APPLE)
    add_custom_target(run
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/pislide
        DEPENDS pislide
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
elseif(UNIX)
    add_custom_target(run
        COMMAND sudo xinit ${CMAKE_CURRENT_BINARY_DIR}/pislide
        DEPENDS pislide
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
