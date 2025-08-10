#include <jni.h>
#include "jni_common.h"
#include "jni_io_stream.h"
#include "jni_audio_format.h"

#include <musac/sdk/decoder.hh>
#include <musac/sdk/decoders_registry.hh>
#include <musac/codecs/register_codecs.hh>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

using namespace musac_jni;

// Global decoder registry
static std::shared_ptr<musac::decoders_registry> g_registry;

extern "C" {

// Initialize the decoder library
JNIEXPORT void JNICALL 
Java_com_musac_MusacNative_nativeInit(JNIEnv* env, jclass clazz) {
    JNI_TRY
        if (!g_registry) {
            g_registry = musac::create_registry_with_all_codecs();
            register_audio_format_classes(env);
        }
    JNI_CATCH_VOID(env)
}

// Detect format from data
JNIEXPORT jstring JNICALL
Java_com_musac_MusacNative_nativeDetectFormat(JNIEnv* env, jclass clazz, jbyteArray data) {
    JNI_TRY
        if (!g_registry) {
            throw_illegal_state(env, "Library not initialized. Call MusacNative.init() first");
            return nullptr;
        }
        
        auto stream = create_stream_from_bytes(env, data);
        auto decoder = g_registry->find_decoder(stream.get());
        
        if (decoder) {
            return string_to_jstring(env, decoder->get_name());
        }
        return nullptr;
    JNI_CATCH(env)
}

// Check if format is supported by extension
JNIEXPORT jboolean JNICALL
Java_com_musac_MusacNative_nativeCanDecodeExtension(JNIEnv* env, jclass clazz, jstring extension) {
    JNI_TRY
        std::string ext = jstring_to_string(env, extension);
        
        // Common audio extensions we support
        static const std::vector<std::string> supported = {
            "wav", "mp3", "ogg", "flac", "aiff", "aif",
            "mod", "s3m", "xm", "it", 
            "mid", "midi", "mus", "xmi", "hmi", "hmp",
            "voc", "vgm", "vgz", "cmf", "opb", "mml"
        };
        
        // Convert to lowercase for comparison
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        return std::find(supported.begin(), supported.end(), ext) != supported.end();
    JNI_CATCH(env)
}

// Create decoder from data (auto-detect format)
JNIEXPORT jlong JNICALL
Java_com_musac_MusacDecoder_nativeCreateFromData(JNIEnv* env, jclass clazz, jbyteArray data) {
    JNI_TRY
        if (!g_registry) {
            throw_illegal_state(env, "Library not initialized. Call MusacNative.init() first");
            return 0;
        }
        
        // Create persistent stream that will be owned by decoder
        auto bytes = jbytearray_to_vector(env, data);
        std::vector<musac::uint8_t> musac_bytes(bytes.begin(), bytes.end());
        auto* stream = new ByteArrayStream(std::move(musac_bytes));
        
        auto decoder = g_registry->find_decoder(stream);
        
        if (!decoder) {
            delete stream;
            throw_io_exception(env, "Could not detect audio format");
            return 0;
        }
        
        // Reset stream and open decoder
        stream->seek(0, musac::seek_origin::set);
        decoder->open(stream);
        
        return create_native_handle(decoder.release());
    JNI_CATCH(env)
}

// Get audio spec
JNIEXPORT jobject JNICALL
Java_com_musac_MusacDecoder_nativeGetSpec(JNIEnv* env, jobject obj, jlong handle) {
    JNI_TRY
        auto* decoder = get_native_handle<musac::decoder>(env, handle);
        if (!decoder) return nullptr;
        
        musac::audio_spec spec;
        spec.format = musac::audio_format::s16le; // Default to S16LE for now
        spec.channels = decoder->get_channels();
        spec.freq = decoder->get_rate();
        
        return audio_spec_to_jobject(env, spec);
    JNI_CATCH(env)
}

// Decode to short array
JNIEXPORT jshortArray JNICALL
Java_com_musac_MusacDecoder_nativeDecodeShort(JNIEnv* env, jobject obj, jlong handle, jint samples) {
    JNI_TRY
        auto* decoder = get_native_handle<musac::decoder>(env, handle);
        if (!decoder) return nullptr;
        
        // First decode to float
        std::vector<float> float_buffer(samples);
        bool call_again = false;
        musac::channels_t channels = decoder->get_channels();
        size_t decoded = decoder->decode(float_buffer.data(), samples, call_again, channels);
        
        if (decoded == 0) {
            return env->NewShortArray(0);
        }
        
        // Convert float to int16_t
        std::vector<musac::int16_t> int_buffer(decoded);
        for (size_t i = 0; i < decoded; i++) {
            float sample = float_buffer[i];
            // Clamp to [-1, 1] range
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            int_buffer[i] = static_cast<musac::int16_t>(sample * 32767.0f);
        }
        
        return short_array_to_jshortarray(env, int_buffer.data(), decoded);
    JNI_CATCH(env)
}

// Decode to float array
JNIEXPORT jfloatArray JNICALL
Java_com_musac_MusacDecoder_nativeDecodeFloat(JNIEnv* env, jobject obj, jlong handle, jint samples) {
    JNI_TRY
        auto* decoder = get_native_handle<musac::decoder>(env, handle);
        if (!decoder) return nullptr;
        
        std::vector<float> float_buffer(samples);
        bool call_again = false;
        musac::channels_t channels = decoder->get_channels();
        size_t decoded = decoder->decode(float_buffer.data(), samples, call_again, channels);
        
        if (decoded == 0) {
            return env->NewFloatArray(0);
        }
        
        return float_array_to_jfloatarray(env, float_buffer.data(), decoded);
    JNI_CATCH(env)
}

// Seek to position
JNIEXPORT void JNICALL
Java_com_musac_MusacDecoder_nativeSeek(JNIEnv* env, jobject obj, jlong handle, jdouble seconds) {
    JNI_TRY
        auto* decoder = get_native_handle<musac::decoder>(env, handle);
        if (!decoder) return;
        
        auto microseconds = std::chrono::microseconds(
            static_cast<std::chrono::microseconds::rep>(seconds * 1000000));
        decoder->seek_to_time(microseconds);
    JNI_CATCH_VOID(env)
}

// Get duration
JNIEXPORT jdouble JNICALL
Java_com_musac_MusacDecoder_nativeGetDuration(JNIEnv* env, jobject obj, jlong handle) {
    JNI_TRY
        auto* decoder = get_native_handle<musac::decoder>(env, handle);
        if (!decoder) return 0;
        
        auto duration_us = decoder->duration();
        return duration_us.count() / 1000000.0;
    JNI_CATCH(env)
}

// Get decoder name
JNIEXPORT jstring JNICALL
Java_com_musac_MusacDecoder_nativeGetName(JNIEnv* env, jobject obj, jlong handle) {
    JNI_TRY
        auto* decoder = get_native_handle<musac::decoder>(env, handle);
        if (!decoder) return nullptr;
        
        return string_to_jstring(env, decoder->get_name());
    JNI_CATCH(env)
}

// Destroy decoder
JNIEXPORT void JNICALL
Java_com_musac_MusacDecoder_nativeDestroy(JNIEnv* env, jobject obj, jlong handle) {
    JNI_TRY
        auto* decoder = get_native_handle<musac::decoder>(env, handle);
        if (decoder) {
            delete decoder;
        }
    JNI_CATCH_VOID(env)
}

} // extern "C"