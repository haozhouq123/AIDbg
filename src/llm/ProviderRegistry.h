#pragma once
#include "ILLMProvider.h"
#include "../utils/ConfigManager.h"

namespace llm {

// Singleton registry that holds all configured providers and dispatches
// requests to the current default provider.
class ProviderRegistry {
public:
    static ProviderRegistry& Instance();

    // Re-build provider list from ConfigManager
    void Initialize();

    // Get the active (default) provider, or nullptr if none configured
    IProvider* GetActive();

    // Get a provider by name (e.g. "glm")
    IProvider* GetByName(const std::string& name);

    // Set the active provider by name
    bool SetActive(const std::string& name);

    // List all provider names
    std::vector<std::string> ListNames() const;

    // Get the active provider name
    std::string GetActiveName() const { return activeName_; }

private:
    ProviderRegistry() = default;
    std::vector<std::unique_ptr<IProvider>> providers_;
    std::string activeName_;
};

} // namespace llm
