#pragma once
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <atomic>


#define VEXYL_PROJECT_ID "1564ff7c-32ab-4c91-95df-ffeae1f38e3b"
#define VEXYL_PUBLIC_TOKEN "eyJhbGciOiJIUzI1NiIsInR5..."
#define VEXYL_VERSION "1.0.0"

namespace Vexyl {

    struct AuthData {
        std::string expiresAt;
        std::string subscription;
        std::map<std::string, std::string> variables;
    };

    class API {
    public:
        // ==========================================
        // EDITABLE CUSTOMER DATA
        // ==========================================
      //  std::string project_id_ = "1564ff7c-32ab-4c91-95df-ffeae1f38e3b";
        //std::string public_token_ = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InRmYXR3aHhzdXprYnFmcG5wa2dhIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzIxMTA5MDIsImV4cCI6MjA4NzY4NjkwMn0.dnjaDvZGZBRHhy-DKKHLwN-40iHJairl-c-9rHcvfOM";
       // std::string version_ = "1.0.0";
        // ==========================================

        API();
        ~API();

        inline void init() {
            InternalInit(VEXYL_PROJECT_ID, VEXYL_PUBLIC_TOKEN, VEXYL_VERSION);
        }

        bool license(const std::string& key, const std::string& hwid = "");

        void start_heartbeat(int interval_seconds = 300, std::function<void(bool valid, const std::string& error)> callback = nullptr);
        void stop_heartbeat();
        bool is_heartbeat_running() const;

        std::string var(const std::string& name) const;
        const std::string& subscription() const;
        const AuthData& data() const;
        const std::string& error() const;

    private:
        AuthData data_;
        std::string last_error_;
        std::string cached_key_;
        std::string cached_hwid_;
        std::atomic<bool> heartbeat_running_{ false };
        void* heartbeat_thread_ = nullptr;
        void InternalInit(std::string projectId, std::string token, std::string version);

        std::string build_payload(const std::string& key, const std::string& hwid) const;
        std::string post_json(const std::string& payload);
    };

}