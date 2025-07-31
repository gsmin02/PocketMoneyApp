//
// Created by ss on 2025-07-31.
//

#include "Transaction.h"
#include <sstream>
#include <stdexcept>

namespace domain {

    std::string transactionTypeToString(TransactionType type) {
        switch (type) {
            case TransactionType::INCOME: return "INCOME";
            case TransactionType::EXPENSE: return "EXPENSE";
            default: return "UNKNOWN";
        }
    }

    TransactionType stringToTransactionType(const std::string& str) {
        if (str == "INCOME") return TransactionType::INCOME;
        if (str == "EXPENSE") return TransactionType::EXPENSE;
        throw std::invalid_argument("Invalid TransactionType string: " + str);
    }

    Transaction::Transaction(int id, int walletId, long long amount,
                             TransactionType type, std::string description, std::string date)
            : id(id), walletId(walletId), amount(amount), type(type),
              description(std::move(description)), date(std::move(date)) {}

    std::string Transaction::toString() const {
        std::stringstream ss;
        ss << "Transaction { id: " << id
           << ", walletId: " << walletId
           << ", amount: " << amount
           << ", type: " << transactionTypeToString(type)
           << ", description: '" << description
           << "', date: '" << date << "' }";
        return ss.str();
    }

}