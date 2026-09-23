#ifndef _JNI_H_INCLUDED_
#define _JNI_H_INCLUDED_

#include <cstdint>
#include <cstddef>

typedef int8_t   jbyte;
typedef int16_t  jshort;
typedef int32_t  jint;
typedef int64_t  jlong;
typedef uint8_t  jboolean;
typedef uint16_t jchar;
typedef float    jfloat;
typedef double   jdouble;

#ifdef __cplusplus
class _jobject {};
class _jclass : public _jobject {};
class _jstring : public _jobject {};
class _jarray : public _jobject {};
class _jobjectArray : public _jarray {};
class _jbyteArray : public _jarray {};
class _jintArray : public _jarray {};
class _jfloatArray : public _jarray {};
typedef _jobject* jobject;
typedef _jclass* jclass;
typedef _jstring* jstring;
typedef _jarray* jarray;
typedef _jobjectArray* jobjectArray;
typedef _jbyteArray* jbyteArray;
typedef _jintArray* jintArray;
typedef _jfloatArray* jfloatArray;
#else
typedef void* jobject;
typedef void* jclass;
typedef void* jstring;
typedef void* jarray;
typedef void* jobjectArray;
typedef void* jbyteArray;
typedef void* jintArray;
typedef void* jfloatArray;
#endif

typedef struct _JNIEnv _JNIEnv;
typedef struct _JavaVM _JavaVM;

#ifdef __cplusplus
extern "C" {
#endif
typedef _JNIEnv* JNIEnv;
typedef _JavaVM* JavaVM;
#ifdef __cplusplus
}
#endif

#define JNI_FALSE 0
#define JNI_TRUE 1

#endif
