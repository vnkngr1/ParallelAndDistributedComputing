cmake_minimum_required(VERSION 3.20)
project(CLionProject)

set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0144 NEW)
cmake_policy(SET CMP0167 OLD)

set(BOOST_ROOT "C:/boost_1_90_0")
set(BOOST_INCLUDEDIR "C:/boost_1_90_0")
set(BOOST_LIBRARYDIR "C:/boost_1_90_0/stage/lib")
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_NO_BOOST_CMAKE ON)
set(Boost_ARCHITECTURE "-x64")
set(Boost_COMPILER "-mgw15")

find_package(Boost REQUIRED COMPONENTS thread)

add_executable(CLionProject main.cpp)

target_link_libraries(CLionProject
        Boost::thread
)