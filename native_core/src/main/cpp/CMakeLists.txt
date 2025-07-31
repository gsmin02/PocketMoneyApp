cmake_minimum_required(VERSION 3.22.1)

project("native_core")

add_library(
        native_core
        SHARED
        native_core.cpp
        sqlite3.c
        domain/Wallet.cpp
        domain/Transaction.cpp
        data/DatabaseHelper.cpp
        data/WalletRepository.cpp
)

target_include_directories(native_core PRIVATE
        src/main/cpp/domain
        src/main/cpp/data
)

find_library(
        log-lib
        log )

target_link_libraries(
        native_core
        log )