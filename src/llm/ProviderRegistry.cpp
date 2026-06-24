#include "ProviderRegistry.h"
#include "GLMProvider.h"
#include "OpenAIProvider.h"
#include "OllamaProvider.h"
#include "../utils/Logger.h"

namespace llm {

ProviderRegistry& ProviderRegistry::Instance() {
    static ProviderRegistry inst;
    return inst;
}

void ProviderRegistry::Initialize() {
    providers_.clear();
    auto& cfg = ConfigManager::Instance().Get();

    for (auto& pc : cfg.providers) {
        if (!pc.enabled) continue;
        if (pc.name == "glm") {
            providers_.push_back(std::make_unique<GLMProvider>(pc));
        } else if (pc.name == "openai" || pc.name == "deepseek" || pc.name == "claude") {
            // OpenAI-compatible APIs (OpenAI / DeepSeek / Claude-via-proxy)
            providers_.push_back(std::make_unique<OpenAIProvider>(pc));
        } else if (pc.name == "ollama") {
            providers_.push_back(std::make_unique<OllamaProvider>(pc));
        }
    }

    if (providers_.empty()) {
        Logger::Warn("No LLM providers configured. Use Settings dialog.");
        return;
    }

    // Pick the active provider
    activeName_ = cfg.defaultProvider;
    if (!GetByName(activeName_)) {
        activeName_ = providers_.front()->Name();
    }

    Logger::Info("LLM providers initialized. Active: " + activeName_);
}

IProvider* ProviderRegistry::GetActive() {
    return GetByName(activeName_);
}

IProvider* ProviderRegistry::GetByName(const std::string& name) {
    for (auto& p : providers_) {
        if (p->Name() == name) return p.get();
    }
    return nullptr;
}

bool ProviderRegistry::SetActive(const std::string& name) {
    if (!GetByName(name)) return false;
    activeName_ = name;
    ConfigManager::Instance().Get().defaultProvider = name;
    ConfigManager::Instance().Save();
    return true;
}

std::vector<std::string> ProviderRegistry::ListNames() const {
    std::vector<std::string> v;
    for (auto& p : providers_) v.push_back(p->Name());
    return v;
}

} // namespace llm
