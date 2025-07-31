//
// Created by ss on 2025-07-31.
//

#ifndef POCKETMONEYAPP_TRANSACTION_H
#define POCKETMONEYAPP_TRANSACTION_H

#include <string>
#include <chrono>
#include <ctime>

namespace domain {

    enum class TransactionType {
        INCOME,
        EXPENSE
    };

    std::string transactionTypeToString(TransactionType type);
    TransactionType stringToTransactionType(const std::string& str);


    class Transaction {
    public:
        int id;
        int walletId;
        long long amount;
        TransactionType type;
        std::string description;
        std::string date;

        Transaction(int id = 0, int walletId = 0, long long amount = 0,
                    TransactionType type = TransactionType::EXPENSE,
                    std::string description = "", std::string date = "");

        std::string toString() const;
    };

}

#endif //POCKETMONEYAPP_TRANSACTION_H
