#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_oblivion_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++! Oblivion Native Android Engine Ready.";
    return env->NewStringUTF(hello.c_str());
}
