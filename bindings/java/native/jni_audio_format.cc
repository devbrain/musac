#include "jni_audio_format.h"

namespace musac_jni {

static jclass audio_format_class = nullptr;
static jmethodID audio_format_constructor = nullptr;
static jfieldID audio_format_value_field = nullptr;

static jclass audio_spec_class = nullptr;
static jmethodID audio_spec_constructor = nullptr;
static jfieldID audio_spec_freq_field = nullptr;
static jfieldID audio_spec_channels_field = nullptr;
static jfieldID audio_spec_format_field = nullptr;

void register_audio_format_classes(JNIEnv* env) {
    // Find and cache AudioFormat class
    jclass local_format_class = env->FindClass("com/musac/AudioFormat");
    if (local_format_class == nullptr) {
        throw_java_exception(env, "java/lang/NoClassDefFoundError", 
                           "Could not find com.musac.AudioFormat");
        return;
    }
    audio_format_class = (jclass)env->NewGlobalRef(local_format_class);
    env->DeleteLocalRef(local_format_class);
    
    // Cache AudioFormat constructor and fields
    audio_format_constructor = env->GetMethodID(audio_format_class, "<init>", "(I)V");
    audio_format_value_field = env->GetFieldID(audio_format_class, "value", "I");
    
    // Find and cache AudioSpec class
    jclass local_spec_class = env->FindClass("com/musac/AudioSpec");
    if (local_spec_class == nullptr) {
        throw_java_exception(env, "java/lang/NoClassDefFoundError",
                           "Could not find com.musac.AudioSpec");
        return;
    }
    audio_spec_class = (jclass)env->NewGlobalRef(local_spec_class);
    env->DeleteLocalRef(local_spec_class);
    
    // Cache AudioSpec constructor and fields
    audio_spec_constructor = env->GetMethodID(audio_spec_class, "<init>", 
                                             "(IILcom/musac/AudioFormat;)V");
    audio_spec_freq_field = env->GetFieldID(audio_spec_class, "freq", "I");
    audio_spec_channels_field = env->GetFieldID(audio_spec_class, "channels", "I");
    audio_spec_format_field = env->GetFieldID(audio_spec_class, "format", 
                                             "Lcom/musac/AudioFormat;");
}

jobject audio_format_to_jobject(JNIEnv* env, musac::audio_format format) {
    if (audio_format_class == nullptr) {
        register_audio_format_classes(env);
    }
    
    return env->NewObject(audio_format_class, audio_format_constructor,
                          static_cast<jint>(format));
}

jobject audio_spec_to_jobject(JNIEnv* env, const musac::audio_spec& spec) {
    if (audio_spec_class == nullptr) {
        register_audio_format_classes(env);
    }
    
    jobject format_obj = audio_format_to_jobject(env, spec.format);
    
    return env->NewObject(audio_spec_class, audio_spec_constructor,
                          static_cast<jint>(spec.freq),
                          static_cast<jint>(spec.channels),
                          format_obj);
}

musac::audio_format jobject_to_audio_format(JNIEnv* env, jobject format_obj) {
    if (format_obj == nullptr) {
        return musac::audio_format::unknown;
    }
    
    jint value = env->GetIntField(format_obj, audio_format_value_field);
    return static_cast<musac::audio_format>(value);
}

musac::audio_spec jobject_to_audio_spec(JNIEnv* env, jobject spec_obj) {
    if (spec_obj == nullptr) {
        return {};
    }
    
    musac::audio_spec spec;
    spec.freq = env->GetIntField(spec_obj, audio_spec_freq_field);
    spec.channels = static_cast<musac::uint8_t>(
        env->GetIntField(spec_obj, audio_spec_channels_field));
    
    jobject format_obj = env->GetObjectField(spec_obj, audio_spec_format_field);
    spec.format = jobject_to_audio_format(env, format_obj);
    
    return spec;
}

} // namespace musac_jni