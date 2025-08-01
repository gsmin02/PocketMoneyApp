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
        char *errMsg = nullptr;
        const char* createWalletsSql =
                "CREATE TABLE IF NOT EXISTS Wallets ("
                "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "NAME TEXT NOT NULL UNIQUE,"
                "DESCRIPTION TEXT,"
                "BALANCE INTEGER DEFAULT 0" // SQLite는 INTEGER로 충분
                ");";

        // 트랜잭션 테이블 생성 SQL 추가
        const char* createTransactionsSql =
                "CREATE TABLE IF NOT EXISTS Transactions ("
                "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "WalletID INTEGER NOT NULL," // 외래키
                "Description TEXT,"
                "Amount INTEGER NOT NULL," // 금액
                "Type INTEGER NOT NULL," // 0: INCOME, 1: EXPENSE
                "TransactionDate TEXT NOT NULL," // YYYY-MM-DD HH:MM:SS
                "FOREIGN KEY(WalletID) REFERENCES Wallets(ID) ON DELETE CASCADE" // 지갑 삭제 시 트랜잭션도 삭제
                ");";

        int rc_wallet = sqlite3_exec(db, createWalletsSql, 0, 0, &errMsg);
        if (rc_wallet != SQLITE_OK) {
            LOGE_DAL("[SQL Error] Wallets table: %s", errMsg);
            sqlite3_free(errMsg);
            return false;
        } else {
            LOGD_DAL("[Info] Wallets table created or already exists.");
        }

        int rc_transaction = sqlite3_exec(db, createTransactionsSql, 0, 0, &errMsg);
        if (rc_transaction != SQLITE_OK) {
            LOGE_DAL("[SQL Error] Transactions table: %s", errMsg);
            sqlite3_free(errMsg);
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