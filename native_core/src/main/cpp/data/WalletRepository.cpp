//
// Created by ss on 2025-07-31.
//

#include "WalletRepository.h"
#include <sstream>
#include <android/log.h>

#define LOG_TAG_REPO "NativeCoreRepo"
#define LOGD_REPO(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_REPO, __VA_ARGS__)
#define LOGE_REPO(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_REPO, __VA_ARGS__)

namespace data {

    WalletRepository::WalletRepository(DatabaseHelper& helper) : dbHelper(helper) {}

    bool WalletRepository::createWallet(const domain::Wallet& wallet) {
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for createWallet.");
            return false;
        }

        std::stringstream ss;
        ss << "INSERT INTO Wallets (NAME, DESCRIPTION, BALANCE) VALUES ('"
           << wallet.name << "', '"
           << wallet.description << "', "
           << wallet.balance << ");";
        std::string sql = ss.str();

        char *zErrMsg = 0;
        int rc = sqlite3_exec(dbHelper.getDb(), sql.c_str(), 0, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (createWallet): %s", zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        }
        LOGD_REPO("Wallet created: %s", wallet.toString().c_str());
        return true;
    }

    std::vector<domain::Wallet> WalletRepository::getAllWallets() {
        std::vector<domain::Wallet> wallets;

        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for getAllWallets.");
            return wallets;
        }

        sqlite3_stmt *stmt;

        const char* sql = "SELECT ID, NAME, DESCRIPTION, BALANCE FROM Wallets;";

        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (getAllWallets prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return wallets;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            domain::Wallet wallet;

            wallet.id = sqlite3_column_int(stmt, 0);
            wallet.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            wallet.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            wallet.balance = sqlite3_column_int64(stmt, 3);

            wallets.push_back(wallet);
        }

        sqlite3_finalize(stmt);

        LOGD_REPO("Retrieved %zu wallets.", wallets.size());
        return wallets;
    }

    bool WalletRepository::updateWallet(const domain::Wallet& wallet) {
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for updateWallet.");
            return false;
        }

        sqlite3_stmt *stmt;
        const char* sql = "UPDATE Wallets SET NAME = ?, DESCRIPTION = ?, BALANCE = ? WHERE ID = ?;";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (updateWallet prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return false;
        }

        // 바인딩: ?에 값을 바인딩
        sqlite3_bind_text(stmt, 1, wallet.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, wallet.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, wallet.balance);
        sqlite3_bind_int(stmt, 4, wallet.id);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            LOGE_REPO("SQL error (updateWallet step): %s", sqlite3_errmsg(dbHelper.getDb()));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        LOGD_REPO("Wallet ID %d updated successfully.", wallet.id);
        return true;
    }

    bool WalletRepository::deleteWallet(int id) {
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for deleteWallet.");
            return false;
        }

        sqlite3_stmt *stmt;
        const char* sql = "DELETE FROM Wallets WHERE ID = ?;";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (deleteWallet prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return false;
        }

        // 바인딩: ?에 ID 값 바인딩
        sqlite3_bind_int(stmt, 1, id);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            LOGE_REPO("SQL error (deleteWallet step): %s", sqlite3_errmsg(dbHelper.getDb()));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        LOGD_REPO("Wallet ID %d deleted successfully.", id);
        return true;
    }

    bool WalletRepository::recalculateBalance(int walletId) {
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for recalculateBalance.");
            return false;
        }

        sqlite3_stmt *stmt;
        const char* sql =
                "SELECT SUM(CASE WHEN Type = 0 THEN Amount ELSE -Amount END) FROM Transactions WHERE wallet_id = ?;";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (recalculateBalance prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return false;
        }

        sqlite3_bind_int(stmt, 1, walletId);

        long long newBalance = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
                newBalance = sqlite3_column_int64(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);
        LOGD_REPO("Calculated new balance for wallet_id %d: %lld", walletId, newBalance);

        sqlite3_stmt *updateStmt;
        const char* updateSql = "UPDATE Wallets SET BALANCE = ? WHERE ID = ?;";
        rc = sqlite3_prepare_v2(dbHelper.getDb(), updateSql, -1, &updateStmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (recalculateBalance update prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return false;
        }

        sqlite3_bind_int64(updateStmt, 1, newBalance);
        sqlite3_bind_int(updateStmt, 2, walletId);

        rc = sqlite3_step(updateStmt);
        if (rc != SQLITE_DONE) {
            LOGE_REPO("SQL error (recalculateBalance update step): %s", sqlite3_errmsg(dbHelper.getDb()));
            sqlite3_finalize(updateStmt);
            return false;
        }

        sqlite3_finalize(updateStmt);
        LOGD_REPO("Wallet ID %d balance updated to %lld successfully.", walletId, newBalance);
        return true;
    }

    domain::Wallet WalletRepository::getWalletById(int id) {
        sqlite3* db = dbHelper.getDb();
        if (!db) {
            LOGE_REPO("Failed to get database connection for getting wallet by ID.");
            return domain::Wallet(); // 기본값 (ID 0)을 반환하여 찾지 못했음을 나타냄
        }

        std::string sql = "SELECT id, name, description, balance FROM wallets WHERE id = ?;";
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            LOGE_REPO("Failed to prepare statement for get wallet by ID: %s", sqlite3_errmsg(db));
            return domain::Wallet();
        }

        sqlite3_bind_int(stmt, 1, id);

        domain::Wallet wallet;
        if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            wallet.id = sqlite3_column_int(stmt, 0);
            wallet.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            wallet.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            wallet.balance = sqlite3_column_int64(stmt, 3);
            LOGD_REPO("getWalletById: Found wallet ID %d: %s", wallet.id, wallet.name.c_str());
        } else {
            LOGD_REPO("getWalletById: Wallet with ID %d not found.", id);
        }

        sqlite3_finalize(stmt);
        return wallet;
    }

}