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
        INCOME = 0, // 수입
        EXPENSE = 1 // 지출
    };

    class Transaction {
    public:
        int id;
        int walletId;
        std::string description;
        long long amount;
        TransactionType type;
        std::string transactionDate;

        Transaction() : id(0), walletId(0), description(""), amount(0), type(TransactionType::EXPENSE), transactionDate("") {}

        Transaction(int id, int walletId, const std::string& description, long long amount, TransactionType type, const std::string& transactionDate)
                : id(id), walletId(walletId), description(description), amount(amount), type(type), transactionDate(transactionDate) {}

        Transaction(int walletId, const std::string& description, long long amount, TransactionType type, const std::string& transactionDate)
                : id(0), walletId(walletId), description(description), amount(amount), type(type), transactionDate(transactionDate) {}
    };

}

#endif //POCKETMONEYAPP_TRANSACTION_H
