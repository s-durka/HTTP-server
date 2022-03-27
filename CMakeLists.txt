cmake_minimum_required(VERSION 3.19)
project(sik_zal1)

set(CMAKE_CXX_STANDARD 17)

add_executable(serwer http_server.cpp SocketTCP.h MessageHTTP.h BufferHTTP.h server_errors.h definitions.h ResponseHTTP.h)
