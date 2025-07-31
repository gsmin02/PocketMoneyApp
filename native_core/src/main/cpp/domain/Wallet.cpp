//
// Created by ss on 2025-07-31.
//

#include "Wallet.h"
#include <sstream>

namespace domain {

    Wallet::Wallet(int id, std::string name, std::string description, long long balance)
            : id(id), name(std::move(name)), description(std::move(description)), balance(balance) {}

    std::string Wallet::toString() const {
        std::stringstream ss;
        ss << "Wallet { id: " << id
           << ", name: '" << name
           << "', description: '" << description
           << "', balance: " << balance << " }";
        return ss.str();
    }

}