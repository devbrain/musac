#ifndef MUSAC_JNI_AUDIO_FORMAT_H
#define MUSAC_JNI_AUDIO_FORMAT_H

#include "jni_common.h"
#include <musac/sdk/audio_format.hh>

namespace musac_jni {

// Convert musac audio_format to Java AudioFormat object
jobject audio_format_to_jobject(JNIEnv* env, musac::audio_format format);

// Convert musac audio_spec to Java AudioSpec object  
jobject audio_spec_to_jobject(JNIEnv* env, const musac::audio_spec& spec);

// Convert Java AudioFormat object to musac audio_format
musac::audio_format jobject_to_audio_format(JNIEnv* env, jobject format_obj);

// Convert Java AudioSpec object to musac audio_spec
musac::audio_spec jobject_to_audio_spec(JNIEnv* env, jobject spec_obj);

// Register AudioFormat and AudioSpec Java classes (call once on library load)
void register_audio_format_classes(JNIEnv* env);

} // namespace musac_jni

#endif // MUSAC_JNI_AUDIO_FORMAT_H