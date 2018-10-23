cmake_minimum_required(VERSION 3.11)
project(Explorer)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.67.0 COMPONENTS system filesystem)

add_executable(Explorer main.cpp chunk_stream.cpp chunk_stream.hpp utils.hpp utils.cpp solid_list_stream.cpp solid_list_stream.hpp texture_pack_stream.cpp texture_pack_stream.hpp DDS.h)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(Explorer LINK_PUBLIC ${Boost_LIBRARIES})
endif ()