//
// Created by ss on 2025-07-31.
//

#ifndef POCKETMONEYAPP_WALLET_H
#define POCKETMONEYAPP_WALLET_H

#include <string>
#include <vector>

namespace domain {

    class Wallet {
    public:
        int id;
        std::string name;
        std::string description;
        long long balance;

        Wallet(int id = 0, std::string name = "", std::string description = "", long long balance = 0);
        std::string toString() const;
    };

}

#endif //POCKETMONEYAPP_WALLET_H
