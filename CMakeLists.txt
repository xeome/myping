cmake_minimum_required(VERSION 3.10)
project(myping)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(AutoCCache)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-protector-all")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O1 -DDEBUG -fsanitize=address -fsanitize=leak")

find_program(LLD_PATH NAMES lld)

# If LLD is found, set it as the linker, because it's faster
if(LLD_PATH)
    message(STATUS "Using LLD as the linker")
    set(CMAKE_LINKER ${LLD_PATH})
else()
    message(STATUS "LLD not found, using the default linker")
endif()

add_subdirectory(loguru)
add_subdirectory(cxxopts)

add_executable(ping main.cpp options.cpp pinger.cpp)
target_link_libraries(ping PRIVATE loguru cxxopts)

install(TARGETS ping DESTINATION bin)