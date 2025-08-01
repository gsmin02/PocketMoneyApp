#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#include "data/DatabaseHelper.h"
#include "data/WalletRepository.h"
#include "data/TransactionRepository.h"
#include "domain/Wallet.h"
#include "domain/Transaction.h"

#define LOG_TAG "NativeCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static data::DatabaseHelper* s_dbHelper = nullptr;
static data::WalletRepository* s_walletRepo = nullptr;
static data::TransactionRepository* s_transactionRepo = nullptr;

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

        s_transactionRepo = new data::TransactionRepository(*s_dbHelper); // <-- TransactionRepository 초기화
        LOGD("TransactionRepository created.");
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

enum JniTransactionType {
    JNI_INCOME = 0,
    JNI_EXPENSE = 1
};

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_createTransactionNative(
        JNIEnv* env,
        jobject /* this */,
        jint walletId,
        jstring descriptionJString,
        jlong amount,
        jint type, // JNI_INCOME (0) or JNI_EXPENSE (1)
        jstring transactionDateJString) {

    if (s_transactionRepo == nullptr || s_walletRepo == nullptr) {
        LOGE("Repositories not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    const char* descCStr = env->GetStringUTFChars(descriptionJString, nullptr);
    const char* dateCStr = env->GetStringUTFChars(transactionDateJString, nullptr);

    std::string description = descCStr;
    std::string transactionDate = dateCStr;

    env->ReleaseStringUTFChars(descriptionJString, descCStr);
    env->ReleaseStringUTFChars(transactionDateJString, dateCStr);

    domain::Transaction newTransaction(
            static_cast<int>(walletId),
            description,
            static_cast<long long>(amount),
            static_cast<domain::TransactionType>(type), // Kotlin Int -> C++ enum
            transactionDate
    );

    bool success = s_transactionRepo->createTransaction(newTransaction);
    if (success) {
        bool balanceUpdated = s_walletRepo->recalculateBalance(static_cast<int>(walletId)); // <-- WalletRepository의 새로운 함수 호출
        if (!balanceUpdated) {
            LOGE("Failed to update wallet balance after creating transaction ID: %d", newTransaction.id);
        }
    }
    LOGD("createTransactionNative result: %s", success ? "true" : "false");
    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_pocketmoneyapp_MainActivity_getTransactionsByWalletNative(
        JNIEnv* env,
        jobject /* this */,
        jint walletId) {

    if (s_transactionRepo == nullptr) {
        LOGE("TransactionRepository not initialized. Call initializeNativeDb first.");
        return nullptr;
    }

    std::vector<domain::Transaction> transactions = s_transactionRepo->getTransactionsByWallet(static_cast<int>(walletId));

    jclass transactionDtoClass = env->FindClass("com/example/pocketmoneyapp/data/TransactionDto");
    if (transactionDtoClass == nullptr) {
        LOGE("Failed to find class 'com/example/pocketmoneyapp/data/TransactionDto'.");
        return nullptr;
    }

    // TransactionDto 생성자 시그니처: (IIJLjava/lang/String;ILjava/lang/String;)V
    // ID(int), WalletID(int), Amount(long), Description(String), Type(int), TransactionDate(String)
    jmethodID constructor = env->GetMethodID(transactionDtoClass, "<init>", "(IIJLjava/lang/String;ILjava/lang/String;)V");
    if (constructor == nullptr) {
        LOGE("Failed to find TransactionDto constructor.");
        env->DeleteLocalRef(transactionDtoClass);
        return nullptr;
    }

    jobjectArray transactionDtoArray = env->NewObjectArray(transactions.size(), transactionDtoClass, nullptr);
    if (transactionDtoArray == nullptr) {
        LOGE("Failed to create jobjectArray for transactions.");
        env->DeleteLocalRef(transactionDtoClass);
        return nullptr;
    }

    for (size_t i = 0; i < transactions.size(); ++i) {
        const auto& transaction = transactions[i];

        jstring descriptionJStr = env->NewStringUTF(transaction.description.c_str());
        jstring transactionDateJStr = env->NewStringUTF(transaction.transactionDate.c_str());

        jobject transactionDtoObj = env->NewObject(transactionDtoClass, constructor,
                                                   static_cast<jint>(transaction.id),
                                                   static_cast<jint>(transaction.walletId),
                                                   static_cast<jlong>(transaction.amount),
                                                   descriptionJStr,
                                                   static_cast<jint>(transaction.type), // C++ enum -> Kotlin Int
                                                   transactionDateJStr);

        env->SetObjectArrayElement(transactionDtoArray, i, transactionDtoObj);

        env->DeleteLocalRef(descriptionJStr);
        env->DeleteLocalRef(transactionDateJStr);
        env->DeleteLocalRef(transactionDtoObj);
    }

    env->DeleteLocalRef(transactionDtoClass);
    LOGD("Successfully retrieved and created %zu TransactionDto objects for wallet ID %d.", transactions.size(), walletId);
    return transactionDtoArray;
}


// updateTransactionNative: 기존 트랜잭션 업데이트 (잔액 변화 미반영 - 이 함수는 주로 설명/날짜 변경용으로 사용)
// 금액 변경이 필요하면 delete 후 create 하는 방식이 잔액 동기화에 더 명확할 수 있습니다.
// 여기서는 간단히 트랜잭션 자체의 필드만 업데이트하고 잔액 재계산은 하지 않습니다.
// 금액 변경이 있을 경우 updateWalletBalanceFromTransactions 함수를 다시 호출해줘야 합니다.
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_updateTransactionNative(
        JNIEnv* env,
        jobject /* this */,
        jint id,
        jint walletId,
        jstring descriptionJString,
        jlong amount,
        jint type,
        jstring transactionDateJString) {

    if (s_transactionRepo == nullptr || s_walletRepo == nullptr) {
        LOGE("Repositories not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    const char* descCStr = env->GetStringUTFChars(descriptionJString, nullptr);
    const char* dateCStr = env->GetStringUTFChars(transactionDateJString, nullptr);

    std::string description = descCStr;
    std::string transactionDate = dateCStr;

    env->ReleaseStringUTFChars(descriptionJString, descCStr);
    env->ReleaseStringUTFChars(transactionDateJString, dateCStr);

    domain::Transaction updatedTransaction(
            static_cast<int>(id),
            static_cast<int>(walletId),
            description,
            static_cast<long long>(amount),
            static_cast<domain::TransactionType>(type),
            transactionDate
    );

    // 원래 트랜잭션 정보를 가져와서 금액이 변경되었는지 확인하는 로직 (선택사항, 복잡도 증가)
    // domain::Transaction oldTransaction = s_transactionRepo->getTransactionById(static_cast<int>(id));
    // if (oldTransaction.id != 0 && (oldTransaction.amount != amount || oldTransaction.type != static_cast<domain::TransactionType>(type))) {
    //     // 금액 또는 타입이 변경되었으므로 잔액 재계산 필요
    //     LOGD("Transaction amount or type changed. Recalculating wallet balance.");
    //     // s_walletRepo->recalculateBalance(static_cast<int>(walletId));
    // }

    bool success = s_transactionRepo->updateTransaction(updatedTransaction);
    if (success) {
        // 트랜잭션이 업데이트된 후, 해당 지갑의 잔액을 재계산하여 일관성을 유지
        bool balanceUpdated = s_walletRepo->recalculateBalance(static_cast<int>(walletId));
        if (!balanceUpdated) {
            LOGE("Failed to update wallet balance after updating transaction ID: %d", id);
        }
    }
    LOGD("updateTransactionNative result: %s", success ? "true" : "false");
    return success ? JNI_TRUE : JNI_FALSE;
}


// deleteTransactionNative: 트랜잭션 삭제 및 지갑 잔액 업데이트
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_pocketmoneyapp_MainActivity_deleteTransactionNative(
        JNIEnv* env,
        jobject /* this */,
        jint id,
        jint walletId) { // 삭제된 트랜잭션의 walletId를 받아서 잔액 재계산에 사용

    if (s_transactionRepo == nullptr || s_walletRepo == nullptr) {
        LOGE("Repositories not initialized. Call initializeNativeDb first.");
        return JNI_FALSE;
    }

    // 트랜잭션 삭제 전, 해당 트랜잭션의 정보를 가져와서 잔액 복구를 위한 데이터를 얻을 수 있습니다.
    // 하지만 가장 견고한 방법은 그냥 지갑의 모든 트랜잭션을 기반으로 잔액을 재계산하는 것입니다.
    bool success = s_transactionRepo->deleteTransaction(static_cast<int>(id));
    if (success) {
        // 트랜잭션 삭제 성공 시, 관련 지갑의 잔액 재계산
        bool balanceUpdated = s_walletRepo->recalculateBalance(static_cast<int>(walletId)); // <-- WalletRepository의 새로운 함수 호출
        if (!balanceUpdated) {
            LOGE("Failed to update wallet balance after deleting transaction ID: %d", id);
        }
    }
    LOGD("deleteTransactionNative for ID %d result: %s", static_cast<int>(id), success ? "true" : "false");
    return success ? JNI_TRUE : JNI_FALSE;
}