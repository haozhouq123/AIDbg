#pragma once
#include "ILLMProvider.h"
#include "../utils/ConfigManager.h"

namespace llm {

// OpenAI-compatible provider. Works for OpenAI, DeepSeek, and any
// service that follows the OpenAI Chat Completions API schema.
class OpenAIProvider : public IProvider {
public:
    explicit OpenAIProvider(const ProviderConfig& cfg) : cfg_(cfg) {}
    std::string Name() const override { return cfg_.name; }
    Response Complete(const Request& req) override;

private:
    ProviderConfig cfg_;
};

} // namespace llm
