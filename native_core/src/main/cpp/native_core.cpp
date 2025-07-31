#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "NativeCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_pocketmoneyapp_MainActivity_stringFromNativeCore(
        JNIEnv* env,
        jobject /* this */) {
    LOGD("stringFromNativeCore JNI function called!");
    std::string hello = "Hello from C++ Native Core!";
    LOGD("stringFromNativeCore JNI function called. Returning: %s", hello.c_str());
    return env->NewStringUTF(hello.c_str());
}
