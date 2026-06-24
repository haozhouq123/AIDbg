#include "OpenAIProvider.h"
#include "../utils/Logger.h"

namespace llm {

Response OpenAIProvider::Complete(const Request& req) {
    Response out;
    if (cfg_.apiKey.empty() && cfg_.name != "ollama") {
        out.error = cfg_.name + " API key not set. Open Settings dialog.";
        return out;
    }

    json_t* body = json_object();
    std::string model = req.model.empty() ? cfg_.model : req.model;
    json_object_set_new(body, "model",       json_string(model.c_str()));
    json_object_set_new(body, "temperature", json_real(req.temperature));
    json_object_set_new(body, "max_tokens",  json_integer(req.maxTokens));
    json_object_set_new(body, "stream",      json_boolean(false));

    json_t* msgs = json_array();
    if (!req.systemPrompt.empty()) {
        json_t* m = json_object();
        json_object_set_new(m, "role",    json_string("system"));
        json_object_set_new(m, "content", json_string(req.systemPrompt.c_str()));
        json_array_append_new(msgs, m);
    }
    for (auto& m : req.messages) {
        json_t* o = json_object();
        json_object_set_new(o, "role",    json_string(m.role.c_str()));
        json_object_set_new(o, "content", json_string(m.content.c_str()));
        json_array_append_new(msgs, o);
    }
    json_object_set_new(body, "messages", msgs);

    // Claude requires max_tokens in a different place; we keep OpenAI-compatible here.
    // For Claude via Anthropic API, use a separate provider class.

    char* dump = json_dumps(body, JSON_COMPACT);
    json_decref(body);
    std::string payload = dump ? dump : "{}";
    free(dump);

    std::string url = cfg_.baseUrl + "/chat/completions";
    http::Headers hdrs = {
        {"Authorization", "Bearer " + cfg_.apiKey},
        {"Content-Type",  "application/json"},
        {"Accept",        "application/json"},
    };

    // DeepSeek and Claude may need custom headers
    if (cfg_.name == "claude") {
        hdrs.push_back({"anthropic-version", "2023-06-01"});
    }

    http::Response hr = http::Post(url, hdrs, payload, req.timeoutSec);
    if (hr.statusCode == 0) {
        out.error = "Network error: " + hr.error;
        return out;
    }
    if (hr.statusCode != 200) {
        json_error_t e;
        json_t* r = json_loads(hr.body.c_str(), 0, &e);
        if (r) {
            json_t* errObj = json_object_get(r, "error");
            if (errObj) {
                json_t* msg = json_object_get(errObj, "message");
                if (msg) out.error = "HTTP " + std::to_string(hr.statusCode) + ": " + json_string_value(msg);
            }
            json_decref(r);
        }
        if (out.error.empty())
            out.error = "HTTP " + std::to_string(hr.statusCode) + ": " + hr.body.substr(0, 256);
        return out;
    }

    json_error_t e;
    json_t* root = json_loads(hr.body.c_str(), 0, &e);
    if (!root) {
        out.error = std::string("JSON parse error: ") + e.text;
        return out;
    }

    json_t* choices = json_object_get(root, "choices");
    if (choices && json_is_array(choices) && json_array_size(choices) > 0) {
        json_t* first = json_array_get(choices, 0);
        json_t* msg   = json_object_get(first, "message");
        if (msg) {
            json_t* content = json_object_get(msg, "content");
            if (content && json_is_string(content))
                out.content = json_string_value(content);
        }
    }

    json_t* usage = json_object_get(root, "usage");
    if (usage) {
        json_t* pt = json_object_get(usage, "prompt_tokens");
        json_t* ct = json_object_get(usage, "completion_tokens");
        if (pt) out.inputTokens  = (int)json_integer_value(pt);
        if (ct) out.outputTokens = (int)json_integer_value(ct);
    }

    json_decref(root);
    if (out.content.empty())
        out.error = "Empty content in " + cfg_.name + " response";
    return out;
}

} // namespace llm
