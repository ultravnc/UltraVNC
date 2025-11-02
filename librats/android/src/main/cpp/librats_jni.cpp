#include <jni.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <android/log.h>
#include "librats_c.h"

#define LOG_TAG "LibRatsJNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global JVM reference for callbacks
static JavaVM* g_jvm = nullptr;
static std::mutex g_callback_mutex;

// Callback data structure
struct CallbackData {
    jobject java_obj;
    jmethodID method_id;
};

// Global maps to store callback data
static std::unordered_map<rats_client_t, CallbackData> g_connection_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_string_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_binary_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_json_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_disconnect_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_peer_discovered_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_file_progress_callbacks;

// GossipSub callback maps
static std::unordered_map<std::string, CallbackData> g_topic_message_callbacks; // key: client_ptr:topic
static std::unordered_map<std::string, CallbackData> g_topic_json_message_callbacks;
static std::unordered_map<std::string, CallbackData> g_topic_peer_joined_callbacks;
static std::unordered_map<std::string, CallbackData> g_topic_peer_left_callbacks;

// File transfer callback maps
static std::unordered_map<rats_client_t, CallbackData> g_file_request_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_directory_request_callbacks;
static std::unordered_map<rats_client_t, CallbackData> g_directory_progress_callbacks;

// Helper to get JNI env
JNIEnv* getJNIEnv() {
    JNIEnv* env = nullptr;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("Failed to attach thread to JVM");
            return nullptr;
        }
    }
    return env;
}

// Helper to create Java string
jstring createJavaString(JNIEnv* env, const char* str) {
    if (!str) return nullptr;
    return env->NewStringUTF(str);
}

// Helper to get C string from Java string
std::string javaStringToCString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

// C callback implementations that bridge to Java
void connection_callback_bridge(void* user_data, const char* peer_id) {
    JNIEnv* env = getJNIEnv();
    if (!env) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = g_connection_callbacks.find(client);
    if (it != g_connection_callbacks.end()) {
        jstring jpeer_id = createJavaString(env, peer_id);
        env->CallVoidMethod(it->second.java_obj, it->second.method_id, jpeer_id);
        env->DeleteLocalRef(jpeer_id);
    }
}

void string_callback_bridge(void* user_data, const char* peer_id, const char* message) {
    JNIEnv* env = getJNIEnv();
    if (!env) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = g_string_callbacks.find(client);
    if (it != g_string_callbacks.end()) {
        jstring jpeer_id = createJavaString(env, peer_id);
        jstring jmessage = createJavaString(env, message);
        env->CallVoidMethod(it->second.java_obj, it->second.method_id, jpeer_id, jmessage);
        env->DeleteLocalRef(jpeer_id);
        env->DeleteLocalRef(jmessage);
    }
}

void binary_callback_bridge(void* user_data, const char* peer_id, const void* data, size_t size) {
    JNIEnv* env = getJNIEnv();
    if (!env) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = g_binary_callbacks.find(client);
    if (it != g_binary_callbacks.end()) {
        jstring jpeer_id = createJavaString(env, peer_id);
        jbyteArray jdata = env->NewByteArray(size);
        env->SetByteArrayRegion(jdata, 0, size, static_cast<const jbyte*>(data));
        env->CallVoidMethod(it->second.java_obj, it->second.method_id, jpeer_id, jdata);
        env->DeleteLocalRef(jpeer_id);
        env->DeleteLocalRef(jdata);
    }
}

void json_callback_bridge(void* user_data, const char* peer_id, const char* json_str) {
    JNIEnv* env = getJNIEnv();
    if (!env) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = g_json_callbacks.find(client);
    if (it != g_json_callbacks.end()) {
        jstring jpeer_id = createJavaString(env, peer_id);
        jstring jjson = createJavaString(env, json_str);
        env->CallVoidMethod(it->second.java_obj, it->second.method_id, jpeer_id, jjson);
        env->DeleteLocalRef(jpeer_id);
        env->DeleteLocalRef(jjson);
    }
}

void disconnect_callback_bridge(void* user_data, const char* peer_id) {
    JNIEnv* env = getJNIEnv();
    if (!env) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = g_disconnect_callbacks.find(client);
    if (it != g_disconnect_callbacks.end()) {
        jstring jpeer_id = createJavaString(env, peer_id);
        env->CallVoidMethod(it->second.java_obj, it->second.method_id, jpeer_id);
        env->DeleteLocalRef(jpeer_id);
    }
}

void peer_discovered_callback_bridge(void* user_data, const char* host, int port, const char* service_name) {
    JNIEnv* env = getJNIEnv();
    if (!env) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = g_peer_discovered_callbacks.find(client);
    if (it != g_peer_discovered_callbacks.end()) {
        jstring jhost = createJavaString(env, host);
        jstring jservice = createJavaString(env, service_name);
        env->CallVoidMethod(it->second.java_obj, it->second.method_id, jhost, port, jservice);
        env->DeleteLocalRef(jhost);
        env->DeleteLocalRef(jservice);
    }
}

void file_progress_callback_bridge(void* user_data, const char* transfer_id, int progress_percent, const char* status) {
    JNIEnv* env = getJNIEnv();
    if (!env) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = g_file_progress_callbacks.find(client);
    if (it != g_file_progress_callbacks.end()) {
        jstring jtransfer_id = createJavaString(env, transfer_id);
        jstring jstatus = createJavaString(env, status);
        env->CallVoidMethod(it->second.java_obj, it->second.method_id, jtransfer_id, progress_percent, jstatus);
        env->DeleteLocalRef(jtransfer_id);
        env->DeleteLocalRef(jstatus);
    }
}

// JNI function implementations
extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    LOGD("LibRats JNI loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGD("LibRats JNI unloaded");
}

// Version functions
JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeGetVersionString(JNIEnv* env, jclass clazz) {
    const char* version = rats_get_version_string();
    return createJavaString(env, version);
}

JNIEXPORT jintArray JNICALL
Java_com_librats_RatsClient_nativeGetVersion(JNIEnv* env, jclass clazz) {
    int major, minor, patch, build;
    rats_get_version(&major, &minor, &patch, &build);
    jintArray result = env->NewIntArray(4);
    jint values[4] = {major, minor, patch, build};
    env->SetIntArrayRegion(result, 0, 4, values);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeGetGitDescribe(JNIEnv* env, jclass clazz) {
    const char* desc = rats_get_git_describe();
    return createJavaString(env, desc);
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeGetAbi(JNIEnv* env, jclass clazz) {
    return rats_get_abi();
}

// Client lifecycle
JNIEXPORT jlong JNICALL
Java_com_librats_RatsClient_nativeCreate(JNIEnv* env, jobject thiz, jint listen_port) {
    rats_client_t client = rats_create(listen_port);
    return reinterpret_cast<jlong>(client);
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeDestroy(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    if (client) {
        // Clean up callbacks
        std::lock_guard<std::mutex> lock(g_callback_mutex);
        g_connection_callbacks.erase(client);
        g_string_callbacks.erase(client);
        g_binary_callbacks.erase(client);
        g_json_callbacks.erase(client);
        g_disconnect_callbacks.erase(client);
        g_peer_discovered_callbacks.erase(client);
        g_file_progress_callbacks.erase(client);
        g_file_request_callbacks.erase(client);
        g_directory_request_callbacks.erase(client);
        g_directory_progress_callbacks.erase(client);
        
        // Clean up topic callbacks for this client
        std::string client_prefix = std::to_string(reinterpret_cast<intptr_t>(client)) + ":";
        auto it = g_topic_message_callbacks.begin();
        while (it != g_topic_message_callbacks.end()) {
            if (it->first.substr(0, client_prefix.length()) == client_prefix) {
                env->DeleteGlobalRef(it->second.java_obj);
                it = g_topic_message_callbacks.erase(it);
            } else {
                ++it;
            }
        }
        
        it = g_topic_json_message_callbacks.begin();
        while (it != g_topic_json_message_callbacks.end()) {
            if (it->first.substr(0, client_prefix.length()) == client_prefix) {
                env->DeleteGlobalRef(it->second.java_obj);
                it = g_topic_json_message_callbacks.erase(it);
            } else {
                ++it;
            }
        }
        
        it = g_topic_peer_joined_callbacks.begin();
        while (it != g_topic_peer_joined_callbacks.end()) {
            if (it->first.substr(0, client_prefix.length()) == client_prefix) {
                env->DeleteGlobalRef(it->second.java_obj);
                it = g_topic_peer_joined_callbacks.erase(it);
            } else {
                ++it;
            }
        }
        
        it = g_topic_peer_left_callbacks.begin();
        while (it != g_topic_peer_left_callbacks.end()) {
            if (it->first.substr(0, client_prefix.length()) == client_prefix) {
                env->DeleteGlobalRef(it->second.java_obj);
                it = g_topic_peer_left_callbacks.erase(it);
            } else {
                ++it;
            }
        }
        
        rats_destroy(client);
    }
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeStart(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_start(client);
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeStop(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    rats_stop(client);
}

// Basic operations
JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeConnect(JNIEnv* env, jobject thiz, jlong client_ptr, jstring host, jint port) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string host_str = javaStringToCString(env, host);
    return rats_connect(client, host_str.c_str(), port);
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeConnectWithStrategy(JNIEnv* env, jobject thiz, jlong client_ptr, jstring host, jint port, jint strategy) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string host_str = javaStringToCString(env, host);
    return rats_connect_with_strategy(client, host_str.c_str(), port, static_cast<rats_connection_strategy_t>(strategy));
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeBroadcastString(JNIEnv* env, jobject thiz, jlong client_ptr, jstring message) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string message_str = javaStringToCString(env, message);
    return rats_broadcast_string(client, message_str.c_str());
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeSendString(JNIEnv* env, jobject thiz, jlong client_ptr, jstring peer_id, jstring message) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string peer_id_str = javaStringToCString(env, peer_id);
    std::string message_str = javaStringToCString(env, message);
    return rats_send_string(client, peer_id_str.c_str(), message_str.c_str());
}

// Binary operations
JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeSendBinary(JNIEnv* env, jobject thiz, jlong client_ptr, jstring peer_id, jbyteArray data) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string peer_id_str = javaStringToCString(env, peer_id);
    
    jbyte* bytes = env->GetByteArrayElements(data, nullptr);
    jsize size = env->GetArrayLength(data);
    
    int result = rats_send_binary(client, peer_id_str.c_str(), bytes, size);
    
    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeBroadcastBinary(JNIEnv* env, jobject thiz, jlong client_ptr, jbyteArray data) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    
    jbyte* bytes = env->GetByteArrayElements(data, nullptr);
    jsize size = env->GetArrayLength(data);
    
    int result = rats_broadcast_binary(client, bytes, size);
    
    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
    return result;
}

// JSON operations
JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeSendJson(JNIEnv* env, jobject thiz, jlong client_ptr, jstring peer_id, jstring json_str) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string peer_id_str = javaStringToCString(env, peer_id);
    std::string json_str_val = javaStringToCString(env, json_str);
    return rats_send_json(client, peer_id_str.c_str(), json_str_val.c_str());
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeBroadcastJson(JNIEnv* env, jobject thiz, jlong client_ptr, jstring json_str) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string json_str_val = javaStringToCString(env, json_str);
    return rats_broadcast_json(client, json_str_val.c_str());
}

// Info functions
JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeGetPeerCount(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_get_peer_count(client);
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeGetListenPort(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_get_listen_port(client);
}

JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeGetOurPeerId(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    char* peer_id = rats_get_our_peer_id(client);
    jstring result = createJavaString(env, peer_id);
    if (peer_id) rats_string_free(peer_id);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeGetConnectionStatisticsJson(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    char* stats = rats_get_connection_statistics_json(client);
    jstring result = createJavaString(env, stats);
    if (stats) rats_string_free(stats);
    return result;
}

JNIEXPORT jobjectArray JNICALL
Java_com_librats_RatsClient_nativeGetPeerIds(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    int count = 0;
    char** peer_ids = rats_get_peer_ids(client, &count);
    
    jobjectArray result = env->NewObjectArray(count, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < count; i++) {
        jstring jstr = createJavaString(env, peer_ids[i]);
        env->SetObjectArrayElement(result, i, jstr);
        env->DeleteLocalRef(jstr);
        rats_string_free(peer_ids[i]);
    }
    if (peer_ids) free(peer_ids);
    
    return result;
}

// Logging controls
JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeSetLoggingEnabled(JNIEnv* env, jclass clazz, jboolean enabled) {
    rats_set_logging_enabled(enabled ? 1 : 0);
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeSetLogLevel(JNIEnv* env, jclass clazz, jstring level) {
    std::string level_str = javaStringToCString(env, level);
    rats_set_log_level(level_str.c_str());
}

// Callback setters
JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeSetConnectionCallback(JNIEnv* env, jobject thiz, jlong client_ptr, jobject callback) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    if (callback) {
        jobject global_ref = env->NewGlobalRef(callback);
        jclass callback_class = env->GetObjectClass(global_ref);
        jmethodID method_id = env->GetMethodID(callback_class, "onConnection", "(Ljava/lang/String;)V");
        
        g_connection_callbacks[client] = {global_ref, method_id};
        rats_set_connection_callback(client, connection_callback_bridge, client);
    } else {
        auto it = g_connection_callbacks.find(client);
        if (it != g_connection_callbacks.end()) {
            env->DeleteGlobalRef(it->second.java_obj);
            g_connection_callbacks.erase(it);
        }
        rats_set_connection_callback(client, nullptr, nullptr);
    }
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeSetStringCallback(JNIEnv* env, jobject thiz, jlong client_ptr, jobject callback) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    if (callback) {
        jobject global_ref = env->NewGlobalRef(callback);
        jclass callback_class = env->GetObjectClass(global_ref);
        jmethodID method_id = env->GetMethodID(callback_class, "onStringMessage", "(Ljava/lang/String;Ljava/lang/String;)V");
        
        g_string_callbacks[client] = {global_ref, method_id};
        rats_set_string_callback(client, string_callback_bridge, client);
    } else {
        auto it = g_string_callbacks.find(client);
        if (it != g_string_callbacks.end()) {
            env->DeleteGlobalRef(it->second.java_obj);
            g_string_callbacks.erase(it);
        }
        rats_set_string_callback(client, nullptr, nullptr);
    }
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeSetBinaryCallback(JNIEnv* env, jobject thiz, jlong client_ptr, jobject callback) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    if (callback) {
        jobject global_ref = env->NewGlobalRef(callback);
        jclass callback_class = env->GetObjectClass(global_ref);
        jmethodID method_id = env->GetMethodID(callback_class, "onBinaryMessage", "(Ljava/lang/String;[B)V");
        
        g_binary_callbacks[client] = {global_ref, method_id};
        rats_set_binary_callback(client, binary_callback_bridge, client);
    } else {
        auto it = g_binary_callbacks.find(client);
        if (it != g_binary_callbacks.end()) {
            env->DeleteGlobalRef(it->second.java_obj);
            g_binary_callbacks.erase(it);
        }
        rats_set_binary_callback(client, nullptr, nullptr);
    }
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeSetJsonCallback(JNIEnv* env, jobject thiz, jlong client_ptr, jobject callback) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    if (callback) {
        jobject global_ref = env->NewGlobalRef(callback);
        jclass callback_class = env->GetObjectClass(global_ref);
        jmethodID method_id = env->GetMethodID(callback_class, "onJsonMessage", "(Ljava/lang/String;Ljava/lang/String;)V");
        
        g_json_callbacks[client] = {global_ref, method_id};
        rats_set_json_callback(client, json_callback_bridge, client);
    } else {
        auto it = g_json_callbacks.find(client);
        if (it != g_json_callbacks.end()) {
            env->DeleteGlobalRef(it->second.java_obj);
            g_json_callbacks.erase(it);
        }
        rats_set_json_callback(client, nullptr, nullptr);
    }
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeSetDisconnectCallback(JNIEnv* env, jobject thiz, jlong client_ptr, jobject callback) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    if (callback) {
        jobject global_ref = env->NewGlobalRef(callback);
        jclass callback_class = env->GetObjectClass(global_ref);
        jmethodID method_id = env->GetMethodID(callback_class, "onDisconnect", "(Ljava/lang/String;)V");
        
        g_disconnect_callbacks[client] = {global_ref, method_id};
        rats_set_disconnect_callback(client, disconnect_callback_bridge, client);
    } else {
        auto it = g_disconnect_callbacks.find(client);
        if (it != g_disconnect_callbacks.end()) {
            env->DeleteGlobalRef(it->second.java_obj);
            g_disconnect_callbacks.erase(it);
        }
        rats_set_disconnect_callback(client, nullptr, nullptr);
    }
}

// Encryption APIs
JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeSetEncryptionEnabled(JNIEnv* env, jobject thiz, jlong client_ptr, jboolean enabled) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_set_encryption_enabled(client, enabled ? 1 : 0);
}

JNIEXPORT jboolean JNICALL
Java_com_librats_RatsClient_nativeIsEncryptionEnabled(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_is_encryption_enabled(client) != 0;
}

JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeGenerateEncryptionKey(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    char* key = rats_generate_encryption_key(client);
    jstring result = createJavaString(env, key);
    if (key) rats_string_free(key);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeSetEncryptionKey(JNIEnv* env, jobject thiz, jlong client_ptr, jstring key) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string key_str = javaStringToCString(env, key);
    return rats_set_encryption_key(client, key_str.c_str());
}

// DHT Discovery
JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeStartDhtDiscovery(JNIEnv* env, jobject thiz, jlong client_ptr, jint dht_port) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_start_dht_discovery(client, dht_port);
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeStopDhtDiscovery(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    rats_stop_dht_discovery(client);
}

JNIEXPORT jboolean JNICALL
Java_com_librats_RatsClient_nativeIsDhtRunning(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_is_dht_running(client) != 0;
}

// mDNS Discovery  
JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeStartMdnsDiscovery(JNIEnv* env, jobject thiz, jlong client_ptr, jstring service_name) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string service_str = javaStringToCString(env, service_name);
    return rats_start_mdns_discovery(client, service_str.c_str());
}

JNIEXPORT void JNICALL
Java_com_librats_RatsClient_nativeStopMdnsDiscovery(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    rats_stop_mdns_discovery(client);
}

JNIEXPORT jboolean JNICALL
Java_com_librats_RatsClient_nativeIsMdnsRunning(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_is_mdns_running(client) != 0;
}

// File Transfer
JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeSendFile(JNIEnv* env, jobject thiz, jlong client_ptr, jstring peer_id, jstring file_path, jstring remote_filename) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string peer_id_str = javaStringToCString(env, peer_id);
    std::string file_path_str = javaStringToCString(env, file_path);
    std::string remote_filename_str = javaStringToCString(env, remote_filename);
    
    char* transfer_id = rats_send_file(client, peer_id_str.c_str(), file_path_str.c_str(), remote_filename_str.c_str());
    jstring result = createJavaString(env, transfer_id);
    if (transfer_id) rats_string_free(transfer_id);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeAcceptFileTransfer(JNIEnv* env, jobject thiz, jlong client_ptr, jstring transfer_id, jstring local_path) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string transfer_id_str = javaStringToCString(env, transfer_id);
    std::string local_path_str = javaStringToCString(env, local_path);
    return rats_accept_file_transfer(client, transfer_id_str.c_str(), local_path_str.c_str());
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeRejectFileTransfer(JNIEnv* env, jobject thiz, jlong client_ptr, jstring transfer_id, jstring reason) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string transfer_id_str = javaStringToCString(env, transfer_id);
    std::string reason_str = javaStringToCString(env, reason);
    return rats_reject_file_transfer(client, transfer_id_str.c_str(), reason_str.c_str());
}

// ===================== GOSSIPSUB FUNCTIONALITY =====================

JNIEXPORT jboolean JNICALL
Java_com_librats_RatsClient_nativeIsGossipsubAvailable(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_is_gossipsub_available(client) != 0;
}

JNIEXPORT jboolean JNICALL
Java_com_librats_RatsClient_nativeIsGossipsubRunning(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_is_gossipsub_running(client) != 0;
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeSubscribeToTopic(JNIEnv* env, jobject thiz, jlong client_ptr, jstring topic) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string topic_str = javaStringToCString(env, topic);
    return rats_subscribe_to_topic(client, topic_str.c_str());
}

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeUnsubscribeFromTopic(JNIEnv* env, jobject thiz, jlong client_ptr, jstring topic) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string topic_str = javaStringToCString(env, topic);
    return rats_unsubscribe_from_topic(client, topic_str.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_librats_RatsClient_nativeIsSubscribedToTopic(JNIEnv* env, jobject thiz, jlong client_ptr, jstring topic) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    std::string topic_str = javaStringToCString(env, topic);
    return rats_is_subscribed_to_topic(client, topic_str.c_str()) != 0;
}

JNIEXPORT jobjectArray JNICALL
Java_com_librats_RatsClient_nativeGetSubscribedTopics(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    int count = 0;
    char** topics = rats_get_subscribed_topics(client, &count);
    
    jobjectArray result = env->NewObjectArray(count, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < count; i++) {
        jstring jstr = createJavaString(env, topics[i]);
        env->SetObjectArrayElement(result, i, jstr);
        env->DeleteLocalRef(jstr);
        rats_string_free(topics[i]);
    }
    if (topics) free(topics);
    
    return result;
}

// ===================== CONFIGURATION PERSISTENCE =====================

JNIEXPORT jint JNICALL
Java_com_librats_RatsClient_nativeLoadConfiguration(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    return rats_load_configuration(client);
}

JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeGetDiscoveryHash(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    char* hash = rats_get_discovery_hash(client);
    jstring result = createJavaString(env, hash);
    if (hash) rats_string_free(hash);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_librats_RatsClient_nativeGetRatsPeerDiscoveryHash(JNIEnv* env, jclass clazz) {
    char* hash = rats_get_rats_peer_discovery_hash();
    jstring result = createJavaString(env, hash);
    if (hash) rats_string_free(hash);
    return result;
}

JNIEXPORT jobjectArray JNICALL
Java_com_librats_RatsClient_nativeGetValidatedPeerIds(JNIEnv* env, jobject thiz, jlong client_ptr) {
    rats_client_t client = reinterpret_cast<rats_client_t>(client_ptr);
    int count = 0;
    char** peer_ids = rats_get_validated_peer_ids(client, &count);
    
    jobjectArray result = env->NewObjectArray(count, env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < count; i++) {
        jstring jstr = createJavaString(env, peer_ids[i]);
        env->SetObjectArrayElement(result, i, jstr);
        env->DeleteLocalRef(jstr);
        rats_string_free(peer_ids[i]);
    }
    if (peer_ids) free(peer_ids);
    
    return result;
}

// Note: This is a condensed implementation showing key functions
// Additional implementations for NAT traversal, ICE coordination, 
// logging, file transfer, and other features would follow the same pattern

} // extern "C"
