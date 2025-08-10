#ifndef MUSAC_JNI_COMMON_H
#define MUSAC_JNI_COMMON_H

#include <jni.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
#include <cstdint>

namespace musac_jni {

// Helper to throw Java exceptions from C++
inline void throw_java_exception(JNIEnv* env, const char* class_name, const std::string& message) {
    jclass exception_class = env->FindClass(class_name);
    if (exception_class != nullptr) {
        env->ThrowNew(exception_class, message.c_str());
        env->DeleteLocalRef(exception_class);
    }
}

// Helper to throw IOException
inline void throw_io_exception(JNIEnv* env, const std::string& message) {
    throw_java_exception(env, "java/io/IOException", message);
}

// Helper to throw IllegalArgumentException
inline void throw_illegal_argument(JNIEnv* env, const std::string& message) {
    throw_java_exception(env, "java/lang/IllegalArgumentException", message);
}

// Helper to throw IllegalStateException
inline void throw_illegal_state(JNIEnv* env, const std::string& message) {
    throw_java_exception(env, "java/lang/IllegalStateException", message);
}

// Convert Java string to C++ string
inline std::string jstring_to_string(JNIEnv* env, jstring jstr) {
    if (jstr == nullptr) {
        return "";
    }
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

// Convert C++ string to Java string
inline jstring string_to_jstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

// Convert Java byte array to C++ vector
inline std::vector<uint8_t> jbytearray_to_vector(JNIEnv* env, jbyteArray array) {
    if (array == nullptr) {
        return {};
    }
    jsize len = env->GetArrayLength(array);
    std::vector<uint8_t> result(len);
    env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte*>(result.data()));
    return result;
}

// Convert C++ vector to Java byte array
inline jbyteArray vector_to_jbytearray(JNIEnv* env, const std::vector<uint8_t>& vec) {
    jbyteArray result = env->NewByteArray(vec.size());
    if (result != nullptr) {
        env->SetByteArrayRegion(result, 0, vec.size(), 
                               reinterpret_cast<const jbyte*>(vec.data()));
    }
    return result;
}

// Convert C++ float array to Java float array
inline jfloatArray float_array_to_jfloatarray(JNIEnv* env, const float* data, size_t size) {
    jfloatArray result = env->NewFloatArray(size);
    if (result != nullptr) {
        env->SetFloatArrayRegion(result, 0, size, data);
    }
    return result;
}

// Convert C++ short array to Java short array
inline jshortArray short_array_to_jshortarray(JNIEnv* env, const int16_t* data, size_t size) {
    jshortArray result = env->NewShortArray(size);
    if (result != nullptr) {
        env->SetShortArrayRegion(result, 0, size, data);
    }
    return result;
}

// RAII wrapper for local references
template<typename T>
class LocalRef {
    JNIEnv* env;
    T ref;
public:
    LocalRef(JNIEnv* e, T r) : env(e), ref(r) {}
    ~LocalRef() { if (ref) env->DeleteLocalRef(ref); }
    operator T() const { return ref; }
    T get() const { return ref; }
};

// Macro for exception-safe JNI methods
#define JNI_TRY try {
#define JNI_CATCH(env) \
    } catch (const std::exception& e) { \
        throw_java_exception(env, "java/lang/RuntimeException", e.what()); \
        return 0; \
    } catch (...) { \
        throw_java_exception(env, "java/lang/RuntimeException", "Unknown error"); \
        return 0; \
    }

#define JNI_CATCH_VOID(env) \
    } catch (const std::exception& e) { \
        throw_java_exception(env, "java/lang/RuntimeException", e.what()); \
        return; \
    } catch (...) { \
        throw_java_exception(env, "java/lang/RuntimeException", "Unknown error"); \
        return; \
    }

// Handle to C++ object stored as Java long
template<typename T>
inline T* get_native_handle(JNIEnv* env, jlong handle) {
    if (handle == 0) {
        throw_illegal_state(env, "Native object has been destroyed");
        return nullptr;
    }
    return reinterpret_cast<T*>(handle);
}

template<typename T>
inline jlong create_native_handle(T* obj) {
    return reinterpret_cast<jlong>(obj);
}

} // namespace musac_jni

#endif // MUSAC_JNI_COMMON_H