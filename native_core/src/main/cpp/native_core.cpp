#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#include "data/DatabaseHelper.h"
#include "data/WalletRepository.h"
#include "data/TransactionRepository.h"
#include "domain/Wallet.h"
#include "domain/Transaction.h"

#define LOG_TAG "NativeCoreJNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static data::DatabaseHelper* s_dbHelper = nullptr;
static data::WalletRepository* s_walletRepo = nullptr;
static data::TransactionRepository* s_transactionRepo = nullptr;

jclass g_walletDtoClass = nullptr;
jmethodID g_walletDtoConstructor = nullptr;
jclass g_transactionDtoClass = nullptr;
jmethodID g_transactionDtoConstructor = nullptr;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnLoad: Could not get JNIEnv");
        return JNI_ERR;
    }

    jclass walletDtoLocalClass = env->FindClass("com/example/pocketmoneyapp/data/WalletDto");
    if (walletDtoLocalClass == nullptr) {
        LOGE("JNI_OnLoad: Failed to find WalletDto class");
        return JNI_ERR;
    }
    g_walletDtoClass = reinterpret_cast<jclass>(env->NewGlobalRef(walletDtoLocalClass));
    if (g_walletDtoClass == nullptr) {
        LOGE("JNI_OnLoad: Failed to create global ref for WalletDto class");
        return JNI_ERR;
    }
    g_walletDtoConstructor = env->GetMethodID(g_walletDtoClass, "<init>", "(ILjava/lang/String;Ljava/lang/String;J)V");
    if (g_walletDtoConstructor == nullptr) {
        LOGE("JNI_OnLoad: Failed to find WalletDto constructor");
        return JNI_ERR;
    }
    env->DeleteLocalRef(walletDtoLocalClass);

    jclass transactionDtoLocalClass = env->FindClass("com/example/pocketmoneyapp/data/TransactionDto");
    if (transactionDtoLocalClass == nullptr) {
        LOGE("JNI_OnLoad: Failed to find TransactionDto class");
        return JNI_ERR;
    }
    g_transactionDtoClass = reinterpret_cast<jclass>(env->NewGlobalRef(transactionDtoLocalClass));
    if (g_transactionDtoClass == nullptr) {
        LOGE("JNI_OnLoad: Failed to create global ref for TransactionDto class");
        return JNI_ERR;
    }
    g_transactionDtoConstructor = env->GetMethodID(g_transactionDtoClass, "<init>", "(IIJLjava/lang/String;ILjava/lang/String;)V");
    if (g_transactionDtoConstructor == nullptr) {
        LOGE("JNI_OnLoad: Failed to find TransactionDto constructor");
        return JNI_ERR;
    }
    env->DeleteLocalRef(transactionDtoLocalClass);

    LOGD("JNI_OnLoad: Classes and constructors loaded successfully.");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnUnload: Could not get JNIEnv");
        return;
    }
    if (g_walletDtoClass != nullptr) {
        env->DeleteGlobalRef(g_walletDtoClass);
        g_walletDtoClass = nullptr;
    }
    if (g_transactionDtoClass != nullptr) {
        env->DeleteGlobalRef(g_transactionDtoClass);
        g_transactionDtoClass = nullptr;
    }
    LOGD("JNI_OnUnload: Global references released.");
}


// MainActivity 관련 JNI 함수들 (companion object가 아니므로 _00024Companion 없음)

extern "C" JNIEXPORT void JNICALL
Java_com_example_pocketmoneyapp_MainActivity_initializeNativeDb(
        JNIEnv* env,
        jobject /* this */,
        jstring dbPathJString) {

    const char* dbPathCStr = env->GetStringUTFChars(dbPathJString, nullptr);
    std::string dbPath = dbPathCStr;
    env->ReleaseStringUTFChars(dbPathJString, dbPathCStr);

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

        s_transactionRepo = new data::TransactionRepository(*s_dbHelper);
        LOGD("TransactionRepository created.");
    } else {
        LOGD("Database already initialized.");
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_createWalletNative(
        JNIEnv* env, jobject /* this */, jstring nameJString, jstring descriptionJString, jlong balance) {
    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    const char* nameCStr = env->GetStringUTFChars(nameJString, nullptr);
    const char* descriptionCStr = env->GetStringUTFChars(descriptionJString, nullptr);

    domain::Wallet newWallet;
    newWallet.name = nameCStr;
    newWallet.description = descriptionCStr;
    newWallet.balance = static_cast<long long>(balance);

    env->ReleaseStringUTFChars(nameJString, nameCStr);
    env->ReleaseStringUTFChars(descriptionJString, descriptionCStr);

    bool success = s_walletRepo->createWallet(newWallet);
    LOGD("createWalletNative: Created wallet: %s, success: %d", newWallet.name.c_str(), success);
    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_pocketmoneyapp_MainActivity_getAllWalletsNative(
        JNIEnv* env, jobject /* this */) {
    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return nullptr;
    }

    std::vector<domain::Wallet> wallets = s_walletRepo->getAllWallets();

    jclass walletDtoClass = g_walletDtoClass;
    if (walletDtoClass == nullptr) {
        LOGE("Failed to get global ref for WalletDto class in getAllWalletsNative.");
        return nullptr;
    }
    jmethodID constructor = g_walletDtoConstructor;
    if (constructor == nullptr) {
        LOGE("Failed to get global ref for WalletDto constructor in getAllWalletsNative.");
        return nullptr;
    }

    jobjectArray walletArray = env->NewObjectArray(wallets.size(), walletDtoClass, nullptr);
    if (walletArray == nullptr) {
        LOGE("Failed to create new jobjectArray for wallets.");
        return nullptr;
    }

    for (size_t i = 0; i < wallets.size(); ++i) {
        jstring nameJStr = env->NewStringUTF(wallets[i].name.c_str());
        jstring descriptionJStr = env->NewStringUTF(wallets[i].description.c_str());

        jobject walletDtoObj = env->NewObject(walletDtoClass, constructor,
                                              static_cast<jint>(wallets[i].id),
                                              nameJStr,
                                              descriptionJStr,
                                              static_cast<jlong>(wallets[i].balance));
        env->SetObjectArrayElement(walletArray, i, walletDtoObj);

        env->DeleteLocalRef(nameJStr);
        env->DeleteLocalRef(descriptionJStr);
        env->DeleteLocalRef(walletDtoObj);
    }
    LOGD("getAllWalletsNative: Retrieved %zu wallets.", wallets.size());
    return walletArray;
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_example_pocketmoneyapp_MainActivity_getWalletByIdNative(
        JNIEnv* env, jobject /* this */, jint id) {

    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return nullptr;
    }

    domain::Wallet wallet = s_walletRepo->getWalletById(static_cast<int>(id));

    if (wallet.id == 0) {
        LOGD("getWalletByIdNative: Wallet with ID %d not found.", static_cast<int>(id));
        return nullptr;
    }

    jclass walletDtoClass = g_walletDtoClass;
    if (walletDtoClass == nullptr) {
        LOGE("Failed to get global ref for WalletDto class in getWalletByIdNative.");
        return nullptr;
    }
    jmethodID constructor = g_walletDtoConstructor;
    if (constructor == nullptr) {
        LOGE("Failed to get global ref for WalletDto constructor in getWalletByIdNative.");
        return nullptr;
    }

    jstring nameJStr = env->NewStringUTF(wallet.name.c_str());
    jstring descriptionJStr = env->NewStringUTF(wallet.description.c_str());

    jobject walletDtoObj = env->NewObject(walletDtoClass, constructor,
                                          static_cast<jint>(wallet.id),
                                          nameJStr,
                                          descriptionJStr,
                                          static_cast<jlong>(wallet.balance));

    env->DeleteLocalRef(nameJStr);
    env->DeleteLocalRef(descriptionJStr);

    LOGD("getWalletByIdNative: Found wallet ID %d: %s, balance %lld", wallet.id, wallet.name.c_str(), wallet.balance);
    return walletDtoObj;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_updateWalletNative(
        JNIEnv* env, jobject /* this */, jint id, jstring nameJString, jstring descriptionJString, jlong balance) {
    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    const char* nameCStr = env->GetStringUTFChars(nameJString, nullptr);
    const char* descriptionCStr = env->GetStringUTFChars(descriptionJString, nullptr);

    domain::Wallet wallet;
    wallet.id = static_cast<int>(id);
    wallet.name = nameCStr;
    wallet.description = descriptionCStr;
    wallet.balance = static_cast<long long>(balance);

    env->ReleaseStringUTFChars(nameJString, nameCStr);
    env->ReleaseStringUTFChars(descriptionJString, descriptionCStr);

    bool success = s_walletRepo->updateWallet(wallet);
    LOGD("updateWalletNative: Updated wallet ID %d, success: %d", wallet.id, success);
    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_deleteWalletNative(
        JNIEnv* env, jobject /* this */, jint id) {
    if (s_walletRepo == nullptr) {
        LOGE("WalletRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    bool success = s_walletRepo->deleteWallet(static_cast<int>(id));
    LOGD("deleteWalletNative: Deleted wallet ID %d, success: %d", static_cast<int>(id), success);
    return success ? JNI_TRUE : JNI_FALSE;
}

// --- TransactionListActivity 관련 JNI 함수들 (companion object가 아니므로 _00024Companion 없음) ---

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_TransactionListActivity_createTransactionNative( // <-- 이름 수정 (00024Companion 제거)
        JNIEnv* env, jobject /* this */, jint walletId, jstring descriptionJString, jlong amount, jint type, jstring transactionDateJString) {

    if (s_transactionRepo == nullptr) {
        LOGE("TransactionRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    const char* descriptionCStr = env->GetStringUTFChars(descriptionJString, nullptr);
    const char* transactionDateCStr = env->GetStringUTFChars(transactionDateJString, nullptr);

    domain::Transaction newTransaction;
    newTransaction.walletId = static_cast<int>(walletId);
    newTransaction.description = descriptionCStr;
    newTransaction.amount = static_cast<long long>(amount);
    newTransaction.type = static_cast<domain::TransactionType>(type);
    newTransaction.transactionDate = transactionDateCStr;

    env->ReleaseStringUTFChars(descriptionJString, descriptionCStr);
    env->ReleaseStringUTFChars(transactionDateJString, transactionDateCStr);

    bool success = s_transactionRepo->createTransaction(newTransaction);
    LOGD("createTransactionNative: Created transaction for wallet ID %d, success: %d", newTransaction.walletId, success);

    if (success && s_walletRepo != nullptr) {
        s_walletRepo->recalculateBalance(newTransaction.walletId);
        LOGD("createTransactionNative: Recalculated balance for wallet ID %d", newTransaction.walletId);
    }
    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_pocketmoneyapp_TransactionListActivity_getTransactionsByWalletNative( // <-- 이름 수정 (00024Companion 제거)
        JNIEnv* env, jobject /* this */, jint walletId) {

    if (s_transactionRepo == nullptr) {
        LOGE("TransactionRepository not initialized. Call initializeNativeDb first.");
        return nullptr;
    }

    std::vector<domain::Transaction> transactions = s_transactionRepo->getTransactionsByWalletId(static_cast<int>(walletId));

    jclass transactionDtoClass = g_transactionDtoClass;
    if (transactionDtoClass == nullptr) {
        LOGE("Failed to get global ref for TransactionDto class in getTransactionsByWalletNative.");
        return nullptr;
    }
    jmethodID constructor = g_transactionDtoConstructor;
    if (constructor == nullptr) {
        LOGE("Failed to get global ref for TransactionDto constructor in getTransactionsByWalletNative.");
        return nullptr;
    }

    jobjectArray transactionArray = env->NewObjectArray(transactions.size(), transactionDtoClass, nullptr);
    if (transactionArray == nullptr) {
        LOGE("Failed to create new jobjectArray for transactions.");
        return nullptr;
    }

    for (size_t i = 0; i < transactions.size(); ++i) {
        jstring descriptionJStr = env->NewStringUTF(transactions[i].description.c_str());
        jstring transactionDateJStr = env->NewStringUTF(transactions[i].transactionDate.c_str());

        jobject transactionDtoObj = env->NewObject(transactionDtoClass, constructor,
                                                   static_cast<jint>(transactions[i].id),
                                                   static_cast<jint>(transactions[i].walletId),
                                                   static_cast<jlong>(transactions[i].amount),
                                                   descriptionJStr,
                                                   static_cast<jint>(transactions[i].type),
                                                   transactionDateJStr);
        env->SetObjectArrayElement(transactionArray, i, transactionDtoObj);

        env->DeleteLocalRef(descriptionJStr);
        env->DeleteLocalRef(transactionDateJStr);
        env->DeleteLocalRef(transactionDtoObj);
    }
    LOGD("getTransactionsByWalletNative: Retrieved %zu transactions for wallet ID %d.", transactions.size(), static_cast<int>(walletId));
    return transactionArray;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_TransactionListActivity_updateTransactionNative( // <-- 이름 수정 (00024Companion 제거)
        JNIEnv* env, jobject /* this */, jint id, jint walletId, jstring descriptionJString, jlong amount, jint type, jstring transactionDateJString) {
    if (s_transactionRepo == nullptr) {
        LOGE("TransactionRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    const char* descriptionCStr = env->GetStringUTFChars(descriptionJString, nullptr);
    const char* transactionDateCStr = env->GetStringUTFChars(transactionDateJString, nullptr);

    domain::Transaction transaction;
    transaction.id = static_cast<int>(id);
    transaction.walletId = static_cast<int>(walletId);
    transaction.description = descriptionCStr;
    transaction.amount = static_cast<long long>(amount);
    transaction.type = static_cast<domain::TransactionType>(type);
    transaction.transactionDate = transactionDateCStr;

    env->ReleaseStringUTFChars(descriptionJString, descriptionCStr);
    env->ReleaseStringUTFChars(transactionDateJString, transactionDateCStr);

    bool success = s_transactionRepo->updateTransaction(transaction);
    LOGD("updateTransactionNative: Updated transaction ID %d for wallet ID %d, success: %d", transaction.id, transaction.walletId, success);

    if (success && s_walletRepo != nullptr) {
        s_walletRepo->recalculateBalance(transaction.walletId);
        LOGD("updateTransactionNative: Recalculated balance for wallet ID %d", transaction.walletId);
    }
    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_TransactionListActivity_deleteTransactionNative( // <-- 이름 수정 (00024Companion 제거)
        JNIEnv* env, jobject /* this */, jint id, jint walletId) {
    if (s_transactionRepo == nullptr) {
        LOGE("TransactionRepository not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    bool success = s_transactionRepo->deleteTransaction(static_cast<int>(id));
    LOGD("deleteTransactionNative: Deleted transaction ID %d for wallet ID %d, success: %d", static_cast<int>(id), static_cast<int>(walletId), success);

    if (success && s_walletRepo != nullptr) {
        s_walletRepo->recalculateBalance(static_cast<int>(walletId));
        LOGD("deleteTransactionNative: Recalculated balance for wallet ID %d", static_cast<int>(walletId));
    }
    return success ? JNI_TRUE : JNI_FALSE;
}