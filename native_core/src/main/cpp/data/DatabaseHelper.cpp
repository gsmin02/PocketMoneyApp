//
// Created by ss on 2025-07-31.
//

#include "DatabaseHelper.h"
#include <android/log.h>

#define LOG_TAG_DAL "NativeCoreDAL"
#define LOGD_DAL(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_DAL, __VA_ARGS__)
#define LOGE_DAL(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_DAL, __VA_ARGS__)

namespace data {

    DatabaseHelper::DatabaseHelper(const std::string& path)
            : dbPath(path), db(nullptr), isOpen(false) {}

    DatabaseHelper::~DatabaseHelper() {
        closeDatabase();
    }

    bool DatabaseHelper::openDatabase() {
        if (isOpen) return true;
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc) {
            LOGE_DAL("[Error] DB 연결 실패: %s", sqlite3_errmsg(db));
            return false;
        } else {
            LOGD_DAL("[Success] DB 연결 성공: %s", dbPath.c_str());
            isOpen = true;
            return true;
        }
    }

    void DatabaseHelper::closeDatabase() {
        if (isOpen && db) {
            sqlite3_close(db);
            LOGD_DAL("[Info] DB 닫힘");
            db = nullptr;
            isOpen = false;
        }
    }

    bool DatabaseHelper::createTables() {
        if (!isOpen || !db) {
            LOGE_DAL("[Error] DB Table 생성 실패");
            return false;
        }
        char *zErrMsg = 0;
        const char *sql_wallet =
                "CREATE TABLE IF NOT EXISTS Wallets ("
                "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "NAME TEXT NOT NULL,"
                "DESCRIPTION TEXT,"
                "BALANCE INTEGER NOT NULL DEFAULT 0);";

        const char *sql_transaction =
                "CREATE TABLE IF NOT EXISTS Transactions ("
                "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "WALLET_ID INTEGER NOT NULL,"
                "AMOUNT INTEGER NOT NULL,"
                "TYPE TEXT NOT NULL," // 'INCOME' or 'EXPENSE'
                "DESCRIPTION TEXT,"
                "DATE TEXT NOT NULL,"
                "FOREIGN KEY (WALLET_ID) REFERENCES Wallets(ID));";

        int rc_wallet = sqlite3_exec(db, sql_wallet, 0, 0, &zErrMsg);
        if (rc_wallet != SQLITE_OK) {
            LOGE_DAL("[SQL Error] Wallets table: %s", zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        } else {
            LOGD_DAL("[Info] Wallets table created or already exists.");
        }

        int rc_transaction = sqlite3_exec(db, sql_transaction, 0, 0, &zErrMsg);
        if (rc_transaction != SQLITE_OK) {
            LOGE_DAL("[SQL Error] Transactions table: %s", zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        } else {
            LOGD_DAL("[Info] Transactions table created or already exists.");
        }
        return true;
    }

    int DatabaseHelper::callback(void *data, int argc, char **argv, char **azColName) {
        // SELECT 쿼리 결과 처리 (디버깅용)
        for (int i = 0; i < argc; i++) {
            LOGD_DAL("%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        return 0;
    }

    sqlite3* DatabaseHelper::getDb() {
        return db;
    }

}