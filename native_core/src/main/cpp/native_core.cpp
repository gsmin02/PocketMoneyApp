#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#include "data/DatabaseHelper.h"
#include "data/WalletRepository.h"
#include "domain/Wallet.h"
#include "domain/Transaction.h"

#define LOG_TAG "NativeCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static data::DatabaseHelper* s_dbHelper = nullptr;
static data::WalletRepository* s_walletRepo = nullptr;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGD("JNI_OnLoad called.");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGD("JNI_OnUnload called.");
    if (s_walletRepo) {
        delete s_walletRepo;
        s_walletRepo = nullptr;
    }
    if (s_dbHelper) {
        s_dbHelper->closeDatabase();
        delete s_dbHelper;
        s_dbHelper = nullptr;
    }
}

// InitializeNativeDb 함수
extern "C" JNIEXPORT void JNICALL
Java_com_example_pocketmoneyapp_MainActivity_initializeNativeDb(
        JNIEnv* env,
        jobject /* this */,
        jstring dbPathJString) {

    const char* dbPathCStr = env->GetStringUTFChars(dbPathJString, nullptr);
    std::string dbPath = dbPathCStr;
    env->ReleaseStringUTFChars(dbPathJString, dbPathCStr);

    LOGD("initializeNativeDb called with path: %s", dbPath.c_str());

    if (s_dbHelper == nullptr) {
        s_dbHelper = new data::DatabaseHelper(dbPath);
        if (!s_dbHelper->openDatabase()) {
            LOGE("Failed to open database at %s", dbPath.c_str());
            return;
        }
        if (!s_dbHelper->createTables()) {
            LOGE("Failed to create tables in database.");
            return;
        }
        LOGD("Database initialized and tables created successfully.");

        s_walletRepo = new data::WalletRepository(*s_dbHelper);
        LOGD("WalletRepository created.");
    } else {
        LOGD("Database already initialized.");
    }
}

// createWalletNative 함수
// Kotlin 에서 WalletDto 의 내용을 받아서 C++ Wallet 객체 생성 및 저장
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_createWalletNative(
        JNIEnv* env,
        jobject /* this */,
        jstring nameJString,
        jstring descriptionJString,
        jlong balance) {

    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    const char* nameCStr = env->GetStringUTFChars(nameJString, nullptr);
    const char* descCStr = env->GetStringUTFChars(descriptionJString, nullptr);

    std::string name = nameCStr;
    std::string description = descCStr;

    env->ReleaseStringUTFChars(nameJString, nameCStr);
    env->ReleaseStringUTFChars(descriptionJString, descCStr);

    domain::Wallet newWallet(0, name, description, static_cast<long long>(balance));

    bool success = s_walletRepo->createWallet(newWallet);
    LOGD("createWalletNative result: %s", success ? "true" : "false");

    return success ? JNI_TRUE : JNI_FALSE;
}

// getAllWalletNamesNative
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_pocketmoneyapp_MainActivity_getAllWalletNamesNative(
        JNIEnv* env,
        jobject /* this */) {

    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return env->NewStringUTF("Error: Native DB not initialized.");
    }

    std::vector<domain::Wallet> wallets = s_walletRepo->getAllWallets();
    std::string result_str = "Wallets:\n";
    if (wallets.empty()) {
        result_str += "No wallets found.";
    } else {
        for (const auto& wallet : wallets) {
            result_str += "- " + wallet.name + " (ID: " + std::to_string(wallet.id) + ", Balance: " + std::to_string(wallet.balance) + ")\n";
        }
    }

    return env->NewStringUTF(result_str.c_str());
}