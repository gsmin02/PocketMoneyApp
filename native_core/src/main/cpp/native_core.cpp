#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#include "data/DatabaseHelper.h"
#include "data/WalletRepository.h"
#include "domain/Wallet.h"

#define LOG_TAG "NativeCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_pocketmoneyapp_MainActivity_stringFromNativeCore(
        JNIEnv* env,
        jobject /* this */) {
    LOGD("stringFromNativeCore JNI function called!");

    std::string result_message;
    std::string db_path = ":memory:";

    data::DatabaseHelper dbHelper(db_path);
    if (!dbHelper.openDatabase()) {
        result_message = "[Error]: DB 연결 실패";
        LOGE("DB 연결 실패: %s", result_message.c_str());
        return env->NewStringUTF(result_message.c_str());
    }

    if (!dbHelper.createTables()) {
        result_message = "[Error] SQL CREATE Table";
        LOGE("[Error] SQL CREATE Table: %s", result_message.c_str());
        return env->NewStringUTF(result_message.c_str());
    }

    data::WalletRepository walletRepo(dbHelper);

    domain::Wallet newWallet(0, "Main Wallet", "My primary spending account", 100000);
    if (walletRepo.createWallet(newWallet)) {
        LOGD("[Success]: Wallet 생성 성공");
        result_message = "[Success]: Wallet 생성 성공";
    } else {
        result_message = "[Error]: Wallet 생성 실패";
        LOGE("[Error]: Wallet 생성 실패: %s", result_message.c_str());
    }

    LOGD("Returning message: %s", result_message.c_str());
    return env->NewStringUTF(result_message.c_str());
}
