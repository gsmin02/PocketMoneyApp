//
// Created by ss on 2025-07-31.
//

#ifndef POCKETMONEYAPP_DATABASEHELPER_H
#define POCKETMONEYAPP_DATABASEHELPER_H

#include "../sqlite3.h"
#include <string>

namespace data {

    class DatabaseHelper {
    private:
        sqlite3 *db;
        std::string dbPath;
        bool isOpen;

    public:
        DatabaseHelper(const std::string& path);
        ~DatabaseHelper();

        bool openDatabase();
        void closeDatabase();
        bool createTables(); // Wallet, Transaction 테이블 생성
        sqlite3* getDb(); // SQLite 인스턴스 반환

        static int callback(void *data, int argc, char **argv, char **azColName);
    };

}

#endif //POCKETMONEYAPP_DATABASEHELPER_H
