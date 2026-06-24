#pragma once
#include "../pluginmain.h"

struct ProviderConfig {
    std::string name;        // e.g. "glm"
    std::string apiKey;
    std::string baseUrl;     // e.g. "https://open.bigmodel.cn/api/paas/v4"
    std::string model;       // e.g. "glm-4.6"
    bool        enabled = true;
};

struct AIDbgConfig {
    std::string              defaultProvider = "glm";
    std::vector<ProviderConfig> providers;
    float                    temperature = 0.2f;
    int                      maxTokens   = 2048;
    bool                     autoInjectContext = true;
    int                      requestTimeoutSec  = 60;

    // Get a provider config by name; returns nullptr if not found
    ProviderConfig* FindProvider(const std::string& name) {
        for (auto& p : providers)
            if (p.name == name) return &p;
        return nullptr;
    }
};

class ConfigManager {
public:
    static ConfigManager& Instance();

    // Load from %APPDATA%\x64dbg\AIDbg\config.json
    // Creates a default config if not present.
    bool Load();

    // Save current config to disk
    bool Save();

    // Get current config (mutable)
    AIDbgConfig& Get() { return cfg_; }
    const AIDbgConfig& Get() const { return cfg_; }

    // Get config file path
    std::string GetConfigPath() const;

    // Reset to defaults (creates a fresh config with all known providers)
    void ResetToDefaults();

private:
    ConfigManager() = default;
    AIDbgConfig cfg_;
};
