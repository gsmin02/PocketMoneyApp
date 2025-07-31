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

}