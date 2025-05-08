
# Configure, build, and run:
# cmake -S . -B build && cmake --build build --target run --parallel

# Need 3.16 for good C++23 support.
cmake_minimum_required(VERSION 3.16)

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

# Add raylib.
target_include_directories(pislide PRIVATE "$ENV{HOME}/others/raylib/src")
target_link_directories(pislide PRIVATE "$ENV{HOME}/others/raylib/src")

# Libraries we need.
target_link_libraries(pislide PRIVATE raylib sqlite3)
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
        COMMAND ./pislide
        DEPENDS pislide
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
elseif(UNIX)
    add_custom_target(run
        COMMAND sudo xinit ./pislide
        DEPENDS pislide
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
