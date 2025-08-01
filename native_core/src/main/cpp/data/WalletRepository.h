//
// Created by ss on 2025-07-31.
//

#ifndef POCKETMONEYAPP_WALLETREPOSITORY_H
#define POCKETMONEYAPP_WALLETREPOSITORY_H

#include "../domain/Wallet.h"
#include "DatabaseHelper.h"
#include <vector>
#include <optional>

namespace data {

    class WalletRepository {
    private:
        DatabaseHelper& dbHelper;

    public:
        explicit WalletRepository(DatabaseHelper& helper);

        bool createWallet(const domain::Wallet& wallet);
        domain::Wallet getWalletById(int id);
        std::vector<domain::Wallet> getAllWallets();
        bool updateWallet(const domain::Wallet& wallet);
        bool deleteWallet(int id);

        bool recalculateBalance(int walletId);
    };

}

#endif //POCKETMONEYAPP_WALLETREPOSITORY_H
