cmake_minimum_required(VERSION 3.16)
project(AudioWaves LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

if(MSVC)
    set(WARN_FLAGS /W4 /WX)
else()
    set(WARN_FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(src)
