#pragma once
#include "ILLMProvider.h"
#include "../utils/ConfigManager.h"

namespace llm {

// 智谱 GLM-4 / GLM-4.6 / GLM-Flash provider
// API: https://open.bigmodel.cn/api/paas/v4/chat/completions
class GLMProvider : public IProvider {
public:
    explicit GLMProvider(const ProviderConfig& cfg) : cfg_(cfg) {}
    std::string Name() const override { return cfg_.name; }
    Response Complete(const Request& req) override;

private:
    ProviderConfig cfg_;
};

} // namespace llm
