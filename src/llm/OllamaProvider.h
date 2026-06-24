#pragma once
#include "ILLMProvider.h"
#include "../utils/ConfigManager.h"

namespace llm {

// Local Ollama provider (http://127.0.0.1:11434)
// API: POST /api/chat
class OllamaProvider : public IProvider {
public:
    explicit OllamaProvider(const ProviderConfig& cfg) : cfg_(cfg) {}
    std::string Name() const override { return cfg_.name; }
    Response Complete(const Request& req) override;

private:
    ProviderConfig cfg_;
};

} // namespace llm
