cmake_minimum_required(VERSION 3.20)
project(CLionProject)

set(CMAKE_CXX_STANDARD 17)

add_executable(CLionProject main.cpp Server.cpp Session.cpp)

target_include_directories(CLionProject PRIVATE "C:/boost_1_90_0")

target_link_libraries(CLionProject PRIVATE
        "C:/boost_1_90_0/stage/lib/libboost_thread-mgw15-mt-d-x64-1_90.a"
        "C:/boost_1_90_0/stage/lib/libboost_chrono-mgw15-mt-d-x64-1_90.a"
        ws2_32
        mswsock
)

target_compile_definitions(CLionProject PRIVATE
        BOOST_ASIO_NO_LIB
        BOOST_ERROR_CODE_HEADER_ONLY
        _WIN32_WINNT=0x0A00
)