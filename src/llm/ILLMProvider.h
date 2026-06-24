#pragma once
#include "../pluginmain.h"
#include "../utils/HttpClient.h"

namespace llm {

struct Message {
    std::string role;     // "system" / "user" / "assistant"
    std::string content;
};

struct Request {
    std::string             systemPrompt;
    std::vector<Message>    messages;
    std::string             model;       // override provider default
    float                   temperature = 0.2f;
    int                     maxTokens   = 2048;
    int                     timeoutSec  = 60;
};

struct Response {
    std::string content;
    int         inputTokens  = 0;
    int         outputTokens = 0;
    std::string error;
    bool        ok() const { return error.empty(); }
};

class IProvider {
public:
    virtual ~IProvider() = default;
    virtual std::string Name() const = 0;
    // Synchronous call (call from a worker thread)
    virtual Response Complete(const Request& req) = 0;
    // Async wrapper
    void CompleteAsync(const Request& req,
                       std::function<void(const Response&)> callback) {
        std::thread([this, req, callback]() {
            Response r = Complete(req);
            if (callback) callback(r);
        }).detach();
    }
};

} // namespace llm
