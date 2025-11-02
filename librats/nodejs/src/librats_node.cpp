#include <napi.h>
#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include "librats_c.h"

using namespace Napi;

// Forward declarations
class RatsClient;

// Global callback storage
struct CallbackData {
    Napi::FunctionReference callback;
    Napi::Env env;
    
    CallbackData(Napi::Env environment) : env(environment) {}
};

std::unordered_map<rats_client_t, std::shared_ptr<CallbackData>> connection_callbacks;
std::unordered_map<rats_client_t, std::shared_ptr<CallbackData>> string_callbacks;
std::unordered_map<rats_client_t, std::shared_ptr<CallbackData>> binary_callbacks;
std::unordered_map<rats_client_t, std::shared_ptr<CallbackData>> json_callbacks;
std::unordered_map<rats_client_t, std::shared_ptr<CallbackData>> disconnect_callbacks;
std::unordered_map<rats_client_t, std::shared_ptr<CallbackData>> file_progress_callbacks;

// C callback wrappers
void connection_callback_wrapper(void* user_data, const char* peer_id) {
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = connection_callbacks.find(client);
    if (it != connection_callbacks.end() && it->second) {
        auto callback_data = it->second;
        callback_data->callback.Call({Napi::String::New(callback_data->env, peer_id)});
    }
}

void string_callback_wrapper(void* user_data, const char* peer_id, const char* message) {
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = string_callbacks.find(client);
    if (it != string_callbacks.end() && it->second) {
        auto callback_data = it->second;
        callback_data->callback.Call({
            Napi::String::New(callback_data->env, peer_id),
            Napi::String::New(callback_data->env, message)
        });
    }
}

void binary_callback_wrapper(void* user_data, const char* peer_id, const void* data, size_t size) {
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = binary_callbacks.find(client);
    if (it != binary_callbacks.end() && it->second) {
        auto callback_data = it->second;
        auto buffer = Napi::Buffer<uint8_t>::New(callback_data->env, size);
        memcpy(buffer.Data(), data, size);
        callback_data->callback.Call({
            Napi::String::New(callback_data->env, peer_id),
            buffer
        });
    }
}

void json_callback_wrapper(void* user_data, const char* peer_id, const char* json_str) {
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = json_callbacks.find(client);
    if (it != json_callbacks.end() && it->second) {
        auto callback_data = it->second;
        callback_data->callback.Call({
            Napi::String::New(callback_data->env, peer_id),
            Napi::String::New(callback_data->env, json_str)
        });
    }
}

void disconnect_callback_wrapper(void* user_data, const char* peer_id) {
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = disconnect_callbacks.find(client);
    if (it != disconnect_callbacks.end() && it->second) {
        auto callback_data = it->second;
        callback_data->callback.Call({Napi::String::New(callback_data->env, peer_id)});
    }
}

void file_progress_callback_wrapper(void* user_data, const char* transfer_id, int progress_percent, const char* status) {
    rats_client_t client = static_cast<rats_client_t>(user_data);
    auto it = file_progress_callbacks.find(client);
    if (it != file_progress_callbacks.end() && it->second) {
        auto callback_data = it->second;
        callback_data->callback.Call({
            Napi::String::New(callback_data->env, transfer_id),
            Napi::Number::New(callback_data->env, progress_percent),
            Napi::String::New(callback_data->env, status)
        });
    }
}

class RatsClient : public Napi::ObjectWrap<RatsClient> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "RatsClient", {
            InstanceMethod("start", &RatsClient::Start),
            InstanceMethod("stop", &RatsClient::Stop),
            InstanceMethod("connect", &RatsClient::Connect),
            InstanceMethod("connectWithStrategy", &RatsClient::ConnectWithStrategy),
            InstanceMethod("disconnect", &RatsClient::Disconnect),
            InstanceMethod("broadcastString", &RatsClient::BroadcastString),
            InstanceMethod("sendString", &RatsClient::SendString),
            InstanceMethod("broadcastBinary", &RatsClient::BroadcastBinary),
            InstanceMethod("sendBinary", &RatsClient::SendBinary),
            InstanceMethod("broadcastJson", &RatsClient::BroadcastJson),
            InstanceMethod("sendJson", &RatsClient::SendJson),
            InstanceMethod("getPeerCount", &RatsClient::GetPeerCount),
            InstanceMethod("getListenPort", &RatsClient::GetListenPort),
            InstanceMethod("getOurPeerId", &RatsClient::GetOurPeerId),
            InstanceMethod("getPeerIds", &RatsClient::GetPeerIds),
            InstanceMethod("getConnectionStatistics", &RatsClient::GetConnectionStatistics),
            InstanceMethod("setMaxPeers", &RatsClient::SetMaxPeers),
            InstanceMethod("getMaxPeers", &RatsClient::GetMaxPeers),
            InstanceMethod("isPeerLimitReached", &RatsClient::IsPeerLimitReached),
            
            // DHT methods
            InstanceMethod("startDhtDiscovery", &RatsClient::StartDhtDiscovery),
            InstanceMethod("stopDhtDiscovery", &RatsClient::StopDhtDiscovery),
            InstanceMethod("isDhtRunning", &RatsClient::IsDhtRunning),
            InstanceMethod("announceForHash", &RatsClient::AnnounceForHash),
            InstanceMethod("getDhtRoutingTableSize", &RatsClient::GetDhtRoutingTableSize),
            
            // mDNS methods
            InstanceMethod("startMdnsDiscovery", &RatsClient::StartMdnsDiscovery),
            InstanceMethod("stopMdnsDiscovery", &RatsClient::StopMdnsDiscovery),
            InstanceMethod("isMdnsRunning", &RatsClient::IsMdnsRunning),
            InstanceMethod("queryMdnsServices", &RatsClient::QueryMdnsServices),
            
            // Encryption methods
            InstanceMethod("setEncryptionEnabled", &RatsClient::SetEncryptionEnabled),
            InstanceMethod("isEncryptionEnabled", &RatsClient::IsEncryptionEnabled),
            InstanceMethod("getEncryptionKey", &RatsClient::GetEncryptionKey),
            InstanceMethod("setEncryptionKey", &RatsClient::SetEncryptionKey),
            InstanceMethod("generateEncryptionKey", &RatsClient::GenerateEncryptionKey),
            
            // GossipSub methods
            InstanceMethod("isGossipsubAvailable", &RatsClient::IsGossipsubAvailable),
            InstanceMethod("isGossipsubRunning", &RatsClient::IsGossipsubRunning),
            InstanceMethod("subscribeToTopic", &RatsClient::SubscribeToTopic),
            InstanceMethod("unsubscribeFromTopic", &RatsClient::UnsubscribeFromTopic),
            InstanceMethod("isSubscribedToTopic", &RatsClient::IsSubscribedToTopic),
            InstanceMethod("getSubscribedTopics", &RatsClient::GetSubscribedTopics),
            InstanceMethod("publishToTopic", &RatsClient::PublishToTopic),
            InstanceMethod("publishJsonToTopic", &RatsClient::PublishJsonToTopic),
            InstanceMethod("getTopicPeers", &RatsClient::GetTopicPeers),
            InstanceMethod("getGossipsubStatistics", &RatsClient::GetGossipsubStatistics),
            
            // File Transfer methods
            InstanceMethod("sendFile", &RatsClient::SendFile),
            InstanceMethod("sendDirectory", &RatsClient::SendDirectory),
            InstanceMethod("requestFile", &RatsClient::RequestFile),
            InstanceMethod("requestDirectory", &RatsClient::RequestDirectory),
            InstanceMethod("acceptFileTransfer", &RatsClient::AcceptFileTransfer),
            InstanceMethod("rejectFileTransfer", &RatsClient::RejectFileTransfer),
            InstanceMethod("cancelFileTransfer", &RatsClient::CancelFileTransfer),
            InstanceMethod("pauseFileTransfer", &RatsClient::PauseFileTransfer),
            InstanceMethod("resumeFileTransfer", &RatsClient::ResumeFileTransfer),
            InstanceMethod("getFileTransferProgress", &RatsClient::GetFileTransferProgress),
            InstanceMethod("getFileTransferStatistics", &RatsClient::GetFileTransferStatistics),
            
            // NAT Traversal methods
            InstanceMethod("discoverPublicIp", &RatsClient::DiscoverPublicIp),
            InstanceMethod("getPublicIp", &RatsClient::GetPublicIp),
            InstanceMethod("detectNatType", &RatsClient::DetectNatType),
            InstanceMethod("getNatCharacteristics", &RatsClient::GetNatCharacteristics),
            InstanceMethod("addIgnoredAddress", &RatsClient::AddIgnoredAddress),
            
            // Callback methods
            InstanceMethod("onConnection", &RatsClient::OnConnection),
            InstanceMethod("onString", &RatsClient::OnString),
            InstanceMethod("onBinary", &RatsClient::OnBinary),
            InstanceMethod("onJson", &RatsClient::OnJson),
            InstanceMethod("onDisconnect", &RatsClient::OnDisconnect),
            InstanceMethod("onFileProgress", &RatsClient::OnFileProgress),
            
            // Configuration persistence
            InstanceMethod("loadConfiguration", &RatsClient::LoadConfiguration),
            InstanceMethod("saveConfiguration", &RatsClient::SaveConfiguration),
            InstanceMethod("setDataDirectory", &RatsClient::SetDataDirectory),
            InstanceMethod("getDataDirectory", &RatsClient::GetDataDirectory)
        });
        
        exports.Set("RatsClient", func);
        return exports;
    }
    
    RatsClient(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RatsClient>(info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsNumber()) {
            Napi::TypeError::New(env, "Expected port number").ThrowAsJavaScriptException();
            return;
        }
        
        int port = info[0].As<Napi::Number>().Int32Value();
        client_ = rats_create(port);
        
        if (!client_) {
            Napi::Error::New(env, "Failed to create RatsClient").ThrowAsJavaScriptException();
            return;
        }
    }
    
    ~RatsClient() {
        if (client_) {
            // Clean up callbacks
            connection_callbacks.erase(client_);
            string_callbacks.erase(client_);
            binary_callbacks.erase(client_);
            json_callbacks.erase(client_);
            disconnect_callbacks.erase(client_);
            file_progress_callbacks.erase(client_);
            
            rats_destroy(client_);
        }
    }

private:
    rats_client_t client_;
    
    // Basic operations
    Napi::Value Start(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int result = rats_start(client_);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    void Stop(const Napi::CallbackInfo& info) {
        rats_stop(client_);
    }
    
    Napi::Value Connect(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
            Napi::TypeError::New(env, "Expected host (string) and port (number)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string host = info[0].As<Napi::String>().Utf8Value();
        int port = info[1].As<Napi::Number>().Int32Value();
        
        int result = rats_connect(client_, host.c_str(), port);
        return Napi::Boolean::New(env, result == 1);
    }
    
    Napi::Value ConnectWithStrategy(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber()) {
            Napi::TypeError::New(env, "Expected host (string), port (number), and strategy (number)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string host = info[0].As<Napi::String>().Utf8Value();
        int port = info[1].As<Napi::Number>().Int32Value();
        rats_connection_strategy_t strategy = static_cast<rats_connection_strategy_t>(info[2].As<Napi::Number>().Int32Value());
        
        rats_error_t result = rats_connect_with_strategy(client_, host.c_str(), port, strategy);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    void Disconnect(const Napi::CallbackInfo& info) {
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(info.Env(), "Expected peer_id (string)").ThrowAsJavaScriptException();
            return;
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        rats_disconnect_peer_by_id(client_, peer_id.c_str());
    }
    
    Napi::Value BroadcastString(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected message (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string message = info[0].As<Napi::String>().Utf8Value();
        int result = rats_broadcast_string(client_, message.c_str());
        return Napi::Number::New(env, result);
    }
    
    Napi::Value SendString(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected peer_id (string) and message (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        std::string message = info[1].As<Napi::String>().Utf8Value();
        
        int result = rats_send_string(client_, peer_id.c_str(), message.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value BroadcastBinary(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsBuffer()) {
            Napi::TypeError::New(env, "Expected data (Buffer)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
        int result = rats_broadcast_binary(client_, buffer.Data(), buffer.Length());
        return Napi::Number::New(env, result);
    }
    
    Napi::Value SendBinary(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBuffer()) {
            Napi::TypeError::New(env, "Expected peer_id (string) and data (Buffer)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        Napi::Buffer<uint8_t> buffer = info[1].As<Napi::Buffer<uint8_t>>();
        
        rats_error_t result = rats_send_binary(client_, peer_id.c_str(), buffer.Data(), buffer.Length());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value BroadcastJson(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected json_str (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string json_str = info[0].As<Napi::String>().Utf8Value();
        int result = rats_broadcast_json(client_, json_str.c_str());
        return Napi::Number::New(env, result);
    }
    
    Napi::Value SendJson(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected peer_id (string) and json_str (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        std::string json_str = info[1].As<Napi::String>().Utf8Value();
        
        rats_error_t result = rats_send_json(client_, peer_id.c_str(), json_str.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    // Info methods
    Napi::Value GetPeerCount(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int count = rats_get_peer_count(client_);
        return Napi::Number::New(env, count);
    }
    
    Napi::Value GetListenPort(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int port = rats_get_listen_port(client_);
        return Napi::Number::New(env, port);
    }
    
    Napi::Value GetOurPeerId(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* peer_id = rats_get_our_peer_id(client_);
        if (!peer_id) return env.Null();
        
        Napi::String result = Napi::String::New(env, peer_id);
        rats_string_free(peer_id);
        return result;
    }
    
    Napi::Value GetPeerIds(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int count = 0;
        char** peer_ids = rats_get_peer_ids(client_, &count);
        
        if (!peer_ids || count == 0) {
            return Napi::Array::New(env, 0);
        }
        
        Napi::Array result = Napi::Array::New(env, count);
        for (int i = 0; i < count; i++) {
            result[i] = Napi::String::New(env, peer_ids[i]);
            rats_string_free(peer_ids[i]);
        }
        free(peer_ids);
        
        return result;
    }
    
    Napi::Value GetConnectionStatistics(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* stats_json = rats_get_connection_statistics_json(client_);
        if (!stats_json) return env.Null();
        
        Napi::String result = Napi::String::New(env, stats_json);
        rats_string_free(stats_json);
        return result;
    }
    
    // Peer configuration
    Napi::Value SetMaxPeers(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsNumber()) {
            Napi::TypeError::New(env, "Expected max_peers (number)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        int max_peers = info[0].As<Napi::Number>().Int32Value();
        rats_error_t result = rats_set_max_peers(client_, max_peers);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value GetMaxPeers(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int max_peers = rats_get_max_peers(client_);
        return Napi::Number::New(env, max_peers);
    }
    
    Napi::Value IsPeerLimitReached(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int reached = rats_is_peer_limit_reached(client_);
        return Napi::Boolean::New(env, reached != 0);
    }
    
    // DHT methods
    Napi::Value StartDhtDiscovery(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsNumber()) {
            Napi::TypeError::New(env, "Expected dht_port (number)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        int dht_port = info[0].As<Napi::Number>().Int32Value();
        rats_error_t result = rats_start_dht_discovery(client_, dht_port);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    void StopDhtDiscovery(const Napi::CallbackInfo& info) {
        rats_stop_dht_discovery(client_);
    }
    
    Napi::Value IsDhtRunning(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int running = rats_is_dht_running(client_);
        return Napi::Boolean::New(env, running != 0);
    }
    
    Napi::Value AnnounceForHash(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
            Napi::TypeError::New(env, "Expected content_hash (string) and port (number)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string content_hash = info[0].As<Napi::String>().Utf8Value();
        int port = info[1].As<Napi::Number>().Int32Value();
        
        rats_error_t result = rats_announce_for_hash(client_, content_hash.c_str(), port);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value GetDhtRoutingTableSize(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        size_t size = rats_get_dht_routing_table_size(client_);
        return Napi::Number::New(env, static_cast<double>(size));
    }
    
    // mDNS methods
    Napi::Value StartMdnsDiscovery(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        const char* service_name = nullptr;
        if (info.Length() > 0 && info[0].IsString()) {
            std::string service = info[0].As<Napi::String>().Utf8Value();
            service_name = service.c_str();
        }
        
        rats_error_t result = rats_start_mdns_discovery(client_, service_name);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    void StopMdnsDiscovery(const Napi::CallbackInfo& info) {
        rats_stop_mdns_discovery(client_);
    }
    
    Napi::Value IsMdnsRunning(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int running = rats_is_mdns_running(client_);
        return Napi::Boolean::New(env, running != 0);
    }
    
    Napi::Value QueryMdnsServices(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        rats_error_t result = rats_query_mdns_services(client_);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    // Encryption methods
    Napi::Value SetEncryptionEnabled(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsBoolean()) {
            Napi::TypeError::New(env, "Expected enabled (boolean)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        bool enabled = info[0].As<Napi::Boolean>().Value();
        rats_error_t result = rats_set_encryption_enabled(client_, enabled ? 1 : 0);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value IsEncryptionEnabled(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int enabled = rats_is_encryption_enabled(client_);
        return Napi::Boolean::New(env, enabled != 0);
    }
    
    Napi::Value GetEncryptionKey(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* key = rats_get_encryption_key(client_);
        if (!key) return env.Null();
        
        Napi::String result = Napi::String::New(env, key);
        rats_string_free(key);
        return result;
    }
    
    Napi::Value SetEncryptionKey(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected key_hex (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string key_hex = info[0].As<Napi::String>().Utf8Value();
        rats_error_t result = rats_set_encryption_key(client_, key_hex.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value GenerateEncryptionKey(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* key = rats_generate_encryption_key(client_);
        if (!key) return env.Null();
        
        Napi::String result = Napi::String::New(env, key);
        rats_string_free(key);
        return result;
    }
    
    // GossipSub methods
    Napi::Value IsGossipsubAvailable(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int available = rats_is_gossipsub_available(client_);
        return Napi::Boolean::New(env, available != 0);
    }
    
    Napi::Value IsGossipsubRunning(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int running = rats_is_gossipsub_running(client_);
        return Napi::Boolean::New(env, running != 0);
    }
    
    Napi::Value SubscribeToTopic(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected topic (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string topic = info[0].As<Napi::String>().Utf8Value();
        rats_error_t result = rats_subscribe_to_topic(client_, topic.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value UnsubscribeFromTopic(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected topic (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string topic = info[0].As<Napi::String>().Utf8Value();
        rats_error_t result = rats_unsubscribe_from_topic(client_, topic.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value IsSubscribedToTopic(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected topic (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string topic = info[0].As<Napi::String>().Utf8Value();
        int subscribed = rats_is_subscribed_to_topic(client_, topic.c_str());
        return Napi::Boolean::New(env, subscribed != 0);
    }
    
    Napi::Value GetSubscribedTopics(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int count = 0;
        char** topics = rats_get_subscribed_topics(client_, &count);
        
        if (!topics || count == 0) {
            return Napi::Array::New(env, 0);
        }
        
        Napi::Array result = Napi::Array::New(env, count);
        for (int i = 0; i < count; i++) {
            result[i] = Napi::String::New(env, topics[i]);
            rats_string_free(topics[i]);
        }
        free(topics);
        
        return result;
    }
    
    Napi::Value PublishToTopic(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected topic (string) and message (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string topic = info[0].As<Napi::String>().Utf8Value();
        std::string message = info[1].As<Napi::String>().Utf8Value();
        
        rats_error_t result = rats_publish_to_topic(client_, topic.c_str(), message.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value PublishJsonToTopic(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected topic (string) and json_str (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string topic = info[0].As<Napi::String>().Utf8Value();
        std::string json_str = info[1].As<Napi::String>().Utf8Value();
        
        rats_error_t result = rats_publish_json_to_topic(client_, topic.c_str(), json_str.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value GetTopicPeers(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected topic (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string topic = info[0].As<Napi::String>().Utf8Value();
        int count = 0;
        char** peers = rats_get_topic_peers(client_, topic.c_str(), &count);
        
        if (!peers || count == 0) {
            return Napi::Array::New(env, 0);
        }
        
        Napi::Array result = Napi::Array::New(env, count);
        for (int i = 0; i < count; i++) {
            result[i] = Napi::String::New(env, peers[i]);
            rats_string_free(peers[i]);
        }
        free(peers);
        
        return result;
    }
    
    Napi::Value GetGossipsubStatistics(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* stats_json = rats_get_gossipsub_statistics_json(client_);
        if (!stats_json) return env.Null();
        
        Napi::String result = Napi::String::New(env, stats_json);
        rats_string_free(stats_json);
        return result;
    }
    
    // File Transfer methods
    Napi::Value SendFile(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected peer_id (string) and file_path (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        std::string file_path = info[1].As<Napi::String>().Utf8Value();
        
        const char* remote_filename = nullptr;
        if (info.Length() > 2 && info[2].IsString()) {
            std::string remote_name = info[2].As<Napi::String>().Utf8Value();
            remote_filename = remote_name.c_str();
        }
        
        char* transfer_id = rats_send_file(client_, peer_id.c_str(), file_path.c_str(), remote_filename);
        if (!transfer_id) return env.Null();
        
        Napi::String result = Napi::String::New(env, transfer_id);
        rats_string_free(transfer_id);
        return result;
    }
    
    Napi::Value SendDirectory(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected peer_id (string) and directory_path (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        std::string directory_path = info[1].As<Napi::String>().Utf8Value();
        
        const char* remote_directory_name = nullptr;
        if (info.Length() > 2 && info[2].IsString()) {
            std::string remote_name = info[2].As<Napi::String>().Utf8Value();
            remote_directory_name = remote_name.c_str();
        }
        
        int recursive = 1;
        if (info.Length() > 3 && info[3].IsBoolean()) {
            recursive = info[3].As<Napi::Boolean>().Value() ? 1 : 0;
        }
        
        char* transfer_id = rats_send_directory(client_, peer_id.c_str(), directory_path.c_str(), remote_directory_name, recursive);
        if (!transfer_id) return env.Null();
        
        Napi::String result = Napi::String::New(env, transfer_id);
        rats_string_free(transfer_id);
        return result;
    }
    
    Napi::Value RequestFile(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
            Napi::TypeError::New(env, "Expected peer_id (string), remote_file_path (string), and local_path (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        std::string remote_file_path = info[1].As<Napi::String>().Utf8Value();
        std::string local_path = info[2].As<Napi::String>().Utf8Value();
        
        char* transfer_id = rats_request_file(client_, peer_id.c_str(), remote_file_path.c_str(), local_path.c_str());
        if (!transfer_id) return env.Null();
        
        Napi::String result = Napi::String::New(env, transfer_id);
        rats_string_free(transfer_id);
        return result;
    }
    
    Napi::Value RequestDirectory(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
            Napi::TypeError::New(env, "Expected peer_id (string), remote_directory_path (string), and local_directory_path (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string peer_id = info[0].As<Napi::String>().Utf8Value();
        std::string remote_directory_path = info[1].As<Napi::String>().Utf8Value();
        std::string local_directory_path = info[2].As<Napi::String>().Utf8Value();
        
        int recursive = 1;
        if (info.Length() > 3 && info[3].IsBoolean()) {
            recursive = info[3].As<Napi::Boolean>().Value() ? 1 : 0;
        }
        
        char* transfer_id = rats_request_directory(client_, peer_id.c_str(), remote_directory_path.c_str(), local_directory_path.c_str(), recursive);
        if (!transfer_id) return env.Null();
        
        Napi::String result = Napi::String::New(env, transfer_id);
        rats_string_free(transfer_id);
        return result;
    }
    
    Napi::Value AcceptFileTransfer(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected transfer_id (string) and local_path (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string transfer_id = info[0].As<Napi::String>().Utf8Value();
        std::string local_path = info[1].As<Napi::String>().Utf8Value();
        
        rats_error_t result = rats_accept_file_transfer(client_, transfer_id.c_str(), local_path.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value RejectFileTransfer(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected transfer_id (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string transfer_id = info[0].As<Napi::String>().Utf8Value();
        
        const char* reason = nullptr;
        if (info.Length() > 1 && info[1].IsString()) {
            std::string reason_str = info[1].As<Napi::String>().Utf8Value();
            reason = reason_str.c_str();
        }
        
        rats_error_t result = rats_reject_file_transfer(client_, transfer_id.c_str(), reason);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value CancelFileTransfer(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected transfer_id (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string transfer_id = info[0].As<Napi::String>().Utf8Value();
        
        rats_error_t result = rats_cancel_file_transfer(client_, transfer_id.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value PauseFileTransfer(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected transfer_id (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string transfer_id = info[0].As<Napi::String>().Utf8Value();
        
        rats_error_t result = rats_pause_file_transfer(client_, transfer_id.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value ResumeFileTransfer(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected transfer_id (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string transfer_id = info[0].As<Napi::String>().Utf8Value();
        
        rats_error_t result = rats_resume_file_transfer(client_, transfer_id.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value GetFileTransferProgress(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected transfer_id (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string transfer_id = info[0].As<Napi::String>().Utf8Value();
        
        char* progress_json = rats_get_file_transfer_progress_json(client_, transfer_id.c_str());
        if (!progress_json) return env.Null();
        
        Napi::String result = Napi::String::New(env, progress_json);
        rats_string_free(progress_json);
        return result;
    }
    
    Napi::Value GetFileTransferStatistics(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* stats_json = rats_get_file_transfer_statistics_json(client_);
        if (!stats_json) return env.Null();
        
        Napi::String result = Napi::String::New(env, stats_json);
        rats_string_free(stats_json);
        return result;
    }
    
    // NAT Traversal methods
    Napi::Value DiscoverPublicIp(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        const char* stun_server = "stun.l.google.com";
        int stun_port = 19302;
        
        if (info.Length() > 0 && info[0].IsString()) {
            std::string server = info[0].As<Napi::String>().Utf8Value();
            stun_server = server.c_str();
        }
        
        if (info.Length() > 1 && info[1].IsNumber()) {
            stun_port = info[1].As<Napi::Number>().Int32Value();
        }
        
        rats_error_t result = rats_discover_and_ignore_public_ip(client_, stun_server, stun_port);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value GetPublicIp(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* public_ip = rats_get_public_ip(client_);
        if (!public_ip) return env.Null();
        
        Napi::String result = Napi::String::New(env, public_ip);
        rats_string_free(public_ip);
        return result;
    }
    
    Napi::Value DetectNatType(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        int nat_type = rats_detect_nat_type(client_);
        return Napi::Number::New(env, nat_type);
    }
    
    Napi::Value GetNatCharacteristics(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* characteristics_json = rats_get_nat_characteristics_json(client_);
        if (!characteristics_json) return env.Null();
        
        Napi::String result = Napi::String::New(env, characteristics_json);
        rats_string_free(characteristics_json);
        return result;
    }
    
    void AddIgnoredAddress(const Napi::CallbackInfo& info) {
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(info.Env(), "Expected ip_address (string)").ThrowAsJavaScriptException();
            return;
        }
        
        std::string ip_address = info[0].As<Napi::String>().Utf8Value();
        rats_add_ignored_address(client_, ip_address.c_str());
    }
    
    // Callback methods
    void OnConnection(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsFunction()) {
            Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
            return;
        }
        
        auto callback_data = std::make_shared<CallbackData>(env);
        callback_data->callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        connection_callbacks[client_] = callback_data;
        rats_set_connection_callback(client_, connection_callback_wrapper, client_);
    }
    
    void OnString(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsFunction()) {
            Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
            return;
        }
        
        auto callback_data = std::make_shared<CallbackData>(env);
        callback_data->callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        string_callbacks[client_] = callback_data;
        rats_set_string_callback(client_, string_callback_wrapper, client_);
    }
    
    void OnBinary(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsFunction()) {
            Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
            return;
        }
        
        auto callback_data = std::make_shared<CallbackData>(env);
        callback_data->callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        binary_callbacks[client_] = callback_data;
        rats_set_binary_callback(client_, binary_callback_wrapper, client_);
    }
    
    void OnJson(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsFunction()) {
            Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
            return;
        }
        
        auto callback_data = std::make_shared<CallbackData>(env);
        callback_data->callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        json_callbacks[client_] = callback_data;
        rats_set_json_callback(client_, json_callback_wrapper, client_);
    }
    
    void OnDisconnect(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsFunction()) {
            Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
            return;
        }
        
        auto callback_data = std::make_shared<CallbackData>(env);
        callback_data->callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        disconnect_callbacks[client_] = callback_data;
        rats_set_disconnect_callback(client_, disconnect_callback_wrapper, client_);
    }
    
    void OnFileProgress(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsFunction()) {
            Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
            return;
        }
        
        auto callback_data = std::make_shared<CallbackData>(env);
        callback_data->callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        file_progress_callbacks[client_] = callback_data;
        rats_set_file_progress_callback(client_, file_progress_callback_wrapper, client_);
    }
    
    // Configuration persistence
    Napi::Value LoadConfiguration(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        rats_error_t result = rats_load_configuration(client_);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value SaveConfiguration(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        rats_error_t result = rats_save_configuration(client_);
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value SetDataDirectory(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected directory_path (string)").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string directory_path = info[0].As<Napi::String>().Utf8Value();
        rats_error_t result = rats_set_data_directory(client_, directory_path.c_str());
        return Napi::Boolean::New(env, result == RATS_SUCCESS);
    }
    
    Napi::Value GetDataDirectory(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        char* directory = rats_get_data_directory(client_);
        if (!directory) return env.Null();
        
        Napi::String result = Napi::String::New(env, directory);
        rats_string_free(directory);
        return result;
    }
};

// Library utility functions
Napi::Value GetVersionString(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    const char* version = rats_get_version_string();
    return Napi::String::New(env, version);
}

Napi::Value GetVersion(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    int major, minor, patch, build;
    rats_get_version(&major, &minor, &patch, &build);
    
    Napi::Object version = Napi::Object::New(env);
    version.Set("major", Napi::Number::New(env, major));
    version.Set("minor", Napi::Number::New(env, minor));
    version.Set("patch", Napi::Number::New(env, patch));
    version.Set("build", Napi::Number::New(env, build));
    
    return version;
}

Napi::Value GetGitDescribe(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    const char* git_describe = rats_get_git_describe();
    return Napi::String::New(env, git_describe);
}

Napi::Value GetAbi(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    uint32_t abi = rats_get_abi();
    return Napi::Number::New(env, abi);
}

// Constants
Napi::Object InitConstants(Napi::Env env) {
    Napi::Object constants = Napi::Object::New(env);
    
    // Error codes
    Napi::Object errors = Napi::Object::New(env);
    errors.Set("SUCCESS", Napi::Number::New(env, RATS_SUCCESS));
    errors.Set("INVALID_HANDLE", Napi::Number::New(env, RATS_ERROR_INVALID_HANDLE));
    errors.Set("INVALID_PARAMETER", Napi::Number::New(env, RATS_ERROR_INVALID_PARAMETER));
    errors.Set("NOT_RUNNING", Napi::Number::New(env, RATS_ERROR_NOT_RUNNING));
    errors.Set("OPERATION_FAILED", Napi::Number::New(env, RATS_ERROR_OPERATION_FAILED));
    errors.Set("PEER_NOT_FOUND", Napi::Number::New(env, RATS_ERROR_PEER_NOT_FOUND));
    errors.Set("MEMORY_ALLOCATION", Napi::Number::New(env, RATS_ERROR_MEMORY_ALLOCATION));
    errors.Set("JSON_PARSE", Napi::Number::New(env, RATS_ERROR_JSON_PARSE));
    constants.Set("ERRORS", errors);
    
    // Connection strategies
    Napi::Object strategies = Napi::Object::New(env);
    strategies.Set("DIRECT_ONLY", Napi::Number::New(env, RATS_STRATEGY_DIRECT_ONLY));
    strategies.Set("STUN_ASSISTED", Napi::Number::New(env, RATS_STRATEGY_STUN_ASSISTED));
    strategies.Set("ICE_FULL", Napi::Number::New(env, RATS_STRATEGY_ICE_FULL));
    strategies.Set("TURN_RELAY", Napi::Number::New(env, RATS_STRATEGY_TURN_RELAY));
    strategies.Set("AUTO_ADAPTIVE", Napi::Number::New(env, RATS_STRATEGY_AUTO_ADAPTIVE));
    constants.Set("CONNECTION_STRATEGIES", strategies);
    
    return constants;
}

// Module initialization
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Export the RatsClient class
    RatsClient::Init(env, exports);
    
    // Export utility functions
    exports.Set("getVersionString", Napi::Function::New(env, GetVersionString));
    exports.Set("getVersion", Napi::Function::New(env, GetVersion));
    exports.Set("getGitDescribe", Napi::Function::New(env, GetGitDescribe));
    exports.Set("getAbi", Napi::Function::New(env, GetAbi));
    
    // Export constants
    exports.Set("constants", InitConstants(env));
    
    return exports;
}

NODE_API_MODULE(librats, Init)
