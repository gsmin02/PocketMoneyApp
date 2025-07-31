cmake_minimum_required(VERSION 3.22.1)

project("native_core")

add_library(
        native_core
        SHARED
        native_core.cpp
        sqlite3.c
)

find_library(
        log-lib
        log )

target_link_libraries(
        native_core
        log )