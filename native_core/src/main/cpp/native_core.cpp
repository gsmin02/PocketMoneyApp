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

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_pocketmoneyapp_MainActivity_getAllWalletsNative(
        JNIEnv* env,
        jobject /* this */) {

    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return nullptr;
    }

    std::vector<domain::Wallet> wallets = s_walletRepo->getAllWallets();

    jclass walletDtoClass = env->FindClass("com/example/pocketmoneyapp/data/WalletDto");
    if (walletDtoClass == nullptr) {
        LOGE("Failed to find class 'com/example/pocketmoneyapp/data/WalletDto'.");
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(walletDtoClass, "<init>", "(ILjava/lang/String;Ljava/lang/String;J)V");
    if (constructor == nullptr) {
        LOGE("Failed to find WalletDto constructor.");
        env->DeleteLocalRef(walletDtoClass);
        return nullptr;
    }

    jobjectArray walletDtoArray = env->NewObjectArray(wallets.size(), walletDtoClass, nullptr);
    if (walletDtoArray == nullptr) {
        LOGE("Failed to create jobjectArray.");
        env->DeleteLocalRef(walletDtoClass);
        return nullptr;
    }

    for (size_t i = 0; i < wallets.size(); ++i) {
        const auto& wallet = wallets[i];

        jstring nameJStr = env->NewStringUTF(wallet.name.c_str());
        jstring descJStr = env->NewStringUTF(wallet.description.c_str());

        jobject walletDtoObj = env->NewObject(walletDtoClass, constructor,
                                              static_cast<jint>(wallet.id),
                                              nameJStr,
                                              descJStr,
                                              static_cast<jlong>(wallet.balance));

        env->SetObjectArrayElement(walletDtoArray, i, walletDtoObj);

        env->DeleteLocalRef(nameJStr);
        env->DeleteLocalRef(descJStr);
        env->DeleteLocalRef(walletDtoObj);
    }

    env->DeleteLocalRef(walletDtoClass);

    LOGD("Successfully retrieved and created %zu WalletDto objects.", wallets.size());
    return walletDtoArray;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_updateWalletNative(
        JNIEnv* env,
        jobject /* this */,
        jint id,
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

    domain::Wallet updatedWallet(static_cast<int>(id), name, description, static_cast<long long>(balance));

    bool success = s_walletRepo->updateWallet(updatedWallet);
    LOGD("updateWalletNative result: %s", success ? "true" : "false");

    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_deleteWalletNative(
        JNIEnv* env,
        jobject /* this */,
        jint id) {

    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    bool success = s_walletRepo->deleteWallet(static_cast<int>(id));
    LOGD("deleteWalletNative for ID %d result: %s", static_cast<int>(id), success ? "true" : "false");

    return success ? JNI_TRUE : JNI_FALSE;
}