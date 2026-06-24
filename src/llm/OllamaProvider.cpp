#include "OllamaProvider.h"
#include "../utils/Logger.h"

namespace llm {

Response OllamaProvider::Complete(const Request& req) {
    Response out;

    // Ollama uses a different schema: { "model", "messages": [...], "stream": false }
    json_t* body = json_object();
    std::string model = req.model.empty() ? cfg_.model : req.model;
    json_object_set_new(body, "model",  json_string(model.c_str()));
    json_object_set_new(body, "stream", json_boolean(false));

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

    json_object_set_new(body, "options", json_object());

    char* dump = json_dumps(body, JSON_COMPACT);
    json_decref(body);
    std::string payload = dump ? dump : "{}";
    free(dump);

    std::string url = cfg_.baseUrl + "/api/chat";
    http::Headers hdrs = {
        {"Content-Type", "application/json"},
        {"Accept",       "application/json"},
    };

    http::Response hr = http::Post(url, hdrs, payload, req.timeoutSec);
    if (hr.statusCode == 0) {
        out.error = "Network error (is Ollama running at " + cfg_.baseUrl + "?): " + hr.error;
        return out;
    }
    if (hr.statusCode != 200) {
        out.error = "HTTP " + std::to_string(hr.statusCode) + ": " + hr.body.substr(0, 256);
        return out;
    }

    json_error_t e;
    json_t* root = json_loads(hr.body.c_str(), 0, &e);
    if (!root) {
        out.error = std::string("JSON parse error: ") + e.text;
        return out;
    }

    json_t* msg = json_object_get(root, "message");
    if (msg) {
        json_t* content = json_object_get(msg, "content");
        if (content && json_is_string(content))
            out.content = json_string_value(content);
    }

    // Ollama provides prompt_eval_count and eval_count
    json_t* pe = json_object_get(root, "prompt_eval_count");
    json_t* ec = json_object_get(root, "eval_count");
    if (pe) out.inputTokens  = (int)json_integer_value(pe);
    if (ec) out.outputTokens = (int)json_integer_value(ec);

    json_decref(root);
    if (out.content.empty())
        out.error = "Empty content in Ollama response";
    return out;
}

} // namespace llm
