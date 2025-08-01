//
// Created by ss on 2025-08-01.
//

#include "TransactionRepository.h"
#include <sqlite3.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <android/log.h>

namespace data {

    TransactionRepository::TransactionRepository(DatabaseHelper& helper) : dbHelper(helper) {
        LOGD_REPO("TransactionRepository initialized.");
    }

    bool TransactionRepository::createTransaction(domain::Transaction& transaction) {
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for createTransaction.");
            return false;
        }

        sqlite3_stmt *stmt;
        const char* sql = "INSERT INTO Transactions (wallet_id, Description, Amount, Type, TransactionDate) VALUES (?, ?, ?, ?, ?);";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (createTransaction prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return false;
        }

        sqlite3_bind_int(stmt, 1, transaction.walletId);
        sqlite3_bind_text(stmt, 2, transaction.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, transaction.amount);
        sqlite3_bind_int(stmt, 4, static_cast<int>(transaction.type));
        sqlite3_bind_text(stmt, 5, transaction.transactionDate.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            LOGE_REPO("SQL error (createTransaction step): %s", sqlite3_errmsg(dbHelper.getDb()));
            sqlite3_finalize(stmt);
            return false;
        }

        transaction.id = static_cast<int>(sqlite3_last_insert_rowid(dbHelper.getDb()));
        sqlite3_finalize(stmt);
        LOGD_REPO("Transaction created successfully with ID: %d", transaction.id);
        return true;
    }

    domain::Transaction TransactionRepository::getTransactionById(int id) {
        domain::Transaction transaction;
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for getTransactionById.");
            return transaction;
        }

        sqlite3_stmt *stmt;
        const char* sql = "SELECT ID, wallet_id, Description, Amount, Type, TransactionDate FROM Transactions WHERE ID = ?;";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (getTransactionById prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return transaction;
        }

        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            transaction.id = sqlite3_column_int(stmt, 0);
            transaction.walletId = sqlite3_column_int(stmt, 1);
            transaction.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            transaction.amount = sqlite3_column_int64(stmt, 3);
            transaction.type = static_cast<domain::TransactionType>(sqlite3_column_int(stmt, 4));
            transaction.transactionDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            LOGD_REPO("Transaction ID %d found.", transaction.id);
        } else {
            LOGD_REPO("Transaction ID %d not found.", id);
        }

        sqlite3_finalize(stmt);
        return transaction;
    }

    std::vector<domain::Transaction> TransactionRepository::getTransactionsByWallet(int walletId, const std::string& orderBy) {
        std::vector<domain::Transaction> transactions;
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for getTransactionsByWallet.");
            return transactions;
        }

        sqlite3_stmt *stmt;
        std::string sql = "SELECT ID, wallet_id, Description, Amount, Type, TransactionDate FROM Transactions WHERE wallet_id = ? ORDER BY " + orderBy + ";";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql.c_str(), -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (getTransactionsByWallet prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return transactions;
        }

        sqlite3_bind_int(stmt, 1, walletId);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            domain::Transaction transaction;
            transaction.id = sqlite3_column_int(stmt, 0);
            transaction.walletId = sqlite3_column_int(stmt, 1);
            transaction.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            transaction.amount = sqlite3_column_int64(stmt, 3);
            transaction.type = static_cast<domain::TransactionType>(sqlite3_column_int(stmt, 4));
            transaction.transactionDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            transactions.push_back(transaction);
        }

        sqlite3_finalize(stmt);
        LOGD_REPO("Retrieved %zu transactions for wallet_id %d.", transactions.size(), walletId);
        return transactions;
    }

    bool TransactionRepository::updateTransaction(const domain::Transaction& transaction) {
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for updateTransaction.");
            return false;
        }

        sqlite3_stmt *stmt;
        const char* sql = "UPDATE Transactions SET wallet_id = ?, Description = ?, Amount = ?, Type = ?, TransactionDate = ? WHERE ID = ?;";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (updateTransaction prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return false;
        }

        sqlite3_bind_int(stmt, 1, transaction.walletId);
        sqlite3_bind_text(stmt, 2, transaction.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, transaction.amount);
        sqlite3_bind_int(stmt, 4, static_cast<int>(transaction.type));
        sqlite3_bind_text(stmt, 5, transaction.transactionDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 6, transaction.id);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            LOGE_REPO("SQL error (updateTransaction step): %s", sqlite3_errmsg(dbHelper.getDb()));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        LOGD_REPO("Transaction ID %d updated successfully.", transaction.id);
        return true;
    }

    bool TransactionRepository::deleteTransaction(int id) {
        if (!dbHelper.getDb()) {
            LOGE_REPO("Database not open for deleteTransaction.");
            return false;
        }

        sqlite3_stmt *stmt;
        const char* sql = "DELETE FROM Transactions WHERE ID = ?;";
        int rc = sqlite3_prepare_v2(dbHelper.getDb(), sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            LOGE_REPO("SQL error (deleteTransaction prepare): %s", sqlite3_errmsg(dbHelper.getDb()));
            return false;
        }

        sqlite3_bind_int(stmt, 1, id);

        int changes_before = sqlite3_total_changes(dbHelper.getDb()); // 변경 전 총 변화 수
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            LOGE_REPO("SQL error (deleteTransaction step): %s", sqlite3_errmsg(dbHelper.getDb()));
            sqlite3_finalize(stmt);
            return false;
        }
        int changes_after = sqlite3_total_changes(dbHelper.getDb()); // 변경 후 총 변화 수

        sqlite3_finalize(stmt);

        if (changes_after > changes_before) { // 변화가 있었다면 성공
            LOGD_REPO("Transaction ID %d deleted successfully.", id);
            return true;
        } else { // 변화가 없다면 (해당 ID가 없거나 실패)
            LOGD_REPO("Transaction ID %d not found or deletion failed.", id);
            return false;
        }
    }

    std::vector<domain::Transaction> TransactionRepository::getTransactionsByWalletId(int walletId) {
        std::vector<domain::Transaction> transactions;
        sqlite3* db = dbHelper.getDb();
        if (!db) {
            LOGE_REPO("Failed to get database connection for getting transactions by wallet ID.");
            return transactions; // 빈 벡터 반환
        }

        std::string sql = "SELECT id, wallet_id, description, amount, type, TransactionDate FROM transactions WHERE wallet_id = ? ORDER BY TransactionDate DESC, id DESC;";
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            LOGE_REPO("Failed to prepare statement for get transactions by wallet ID: %s", sqlite3_errmsg(db));
            return transactions;
        }

        sqlite3_bind_int(stmt, 1, walletId);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            domain::Transaction transaction;
            transaction.id = sqlite3_column_int(stmt, 0);
            transaction.walletId = sqlite3_column_int(stmt, 1);
            transaction.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            transaction.amount = sqlite3_column_int64(stmt, 3);
            transaction.type = static_cast<domain::TransactionType>(sqlite3_column_int(stmt, 4));
            transaction.transactionDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            transactions.push_back(transaction);
        }

        if (rc != SQLITE_DONE) {
            LOGE_REPO("Failed to execute statement for get transactions by wallet ID: %s", sqlite3_errmsg(db));
        }

        sqlite3_finalize(stmt);
        LOGD_REPO("Retrieved %zu transactions for wallet ID %d.", transactions.size(), walletId);
        return transactions;
    }

}