#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#define LOG_TAG "NativeCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_pocketmoneyapp_MainActivity_stringFromNativeCore(
        JNIEnv* env,
        jobject /* this */) {
    LOGD("stringFromNativeCore JNI function called!");

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    const char *sql;
    std::string result_message;

    rc = sqlite3_open(":memory:", &db);

    if (rc) {
        LOGE("DB 연결 실패: %s", sqlite3_errmsg(db));
        result_message = "[Error]: DB 연결 실패";
    } else {
        LOGD("[Success]: DB 연결 성공");

        sql = "CREATE TABLE COMPANY(ID INT PRIMARY KEY NOT NULL, NAME TEXT NOT NULL);";
        rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            LOGE("[Error] SQL CREATE: %s", zErrMsg);
            sqlite3_free(zErrMsg);
            result_message = "[Error] SQL CREATE Test";
        } else {
            LOGD("[Success]: Table 생성 성공");

            sql = "INSERT INTO COMPANY (ID,NAME) VALUES (1, 'Alice');" \
                  "INSERT INTO COMPANY (ID,NAME) VALUES (2, 'Bob');";
            rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);

            if (rc != SQLITE_OK) {
                LOGE("[Error] SQL INSERT: %s", zErrMsg);
                sqlite3_free(zErrMsg);
                result_message = "[Error] SQL INSERT Test";
            } else {
                LOGD("[Success]: Record 생성 성공");
                result_message = "[Success]: DB Test 성공";
            }
        }
        sqlite3_close(db);
    }

    LOGD("Returning message: %s", result_message.c_str());
    return env->NewStringUTF(result_message.c_str());
}
