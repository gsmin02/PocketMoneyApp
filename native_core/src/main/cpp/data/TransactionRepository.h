//
// Created by ss on 2025-08-01.
//

#ifndef POCKETMONEYAPP_TRANSACTIONREPOSITORY_H
#define POCKETMONEYAPP_TRANSACTIONREPOSITORY_H

#include <vector>
#include "../domain/Transaction.h"
#include "DatabaseHelper.h"

#ifndef LOG_REPO_TAG
#define LOG_REPO_TAG "TransactionRepo"
#define LOGD_REPO(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_REPO_TAG, __VA_ARGS__)
#define LOGE_REPO(...) __android_log_print(ANDROID_LOG_ERROR, LOG_REPO_TAG, __VA_ARGS__)
#endif

namespace data {

    class TransactionRepository {
    private:
        DatabaseHelper& dbHelper;

    public:
        explicit TransactionRepository(DatabaseHelper& helper);

        bool createTransaction(domain::Transaction& transaction);

        domain::Transaction getTransactionById(int id);

        std::vector<domain::Transaction> getTransactionsByWallet(int walletId, const std::string& orderBy = "transactionDate DESC");

        bool updateTransaction(const domain::Transaction& transaction);

        bool deleteTransaction(int id);
    };

}

#endif //POCKETMONEYAPP_TRANSACTIONREPOSITORY_H
