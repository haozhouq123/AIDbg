#include "ConfigManager.h"
#include "Logger.h"
#include <shlobj.h>

ConfigManager& ConfigManager::Instance() {
    static ConfigManager inst;
    return inst;
}

std::string ConfigManager::GetConfigPath() const {
    char appData[MAX_PATH] = {0};
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appData) != S_OK) {
        // Fallback: use TEMP
        GetTempPathA(MAX_PATH, appData);
    }
    std::string dir = std::string(appData) + "\\x64dbg\\AIDbg";
    CreateDirectoryA(dir.c_str(), NULL);
    return dir + "\\config.json";
}

void ConfigManager::ResetToDefaults() {
    cfg_ = AIDbgConfig{};
    cfg_.providers.clear();

    // GLM (智谱)
    ProviderConfig glm;
    glm.name    = "glm";
    glm.apiKey  = "";
    glm.baseUrl = "https://open.bigmodel.cn/api/paas/v4";
    glm.model   = "glm-4.6";
    glm.enabled = true;
    cfg_.providers.push_back(glm);

    // OpenAI
    ProviderConfig openai;
    openai.name    = "openai";
    openai.apiKey  = "";
    openai.baseUrl = "https://api.openai.com/v1";
    openai.model   = "gpt-4o-mini";
    openai.enabled = true;
    cfg_.providers.push_back(openai);

    // DeepSeek
    ProviderConfig deepseek;
    deepseek.name    = "deepseek";
    deepseek.apiKey  = "";
    deepseek.baseUrl = "https://api.deepseek.com/v1";
    deepseek.model   = "deepseek-coder";
    deepseek.enabled = true;
    cfg_.providers.push_back(deepseek);

    // Claude (Anthropic)
    ProviderConfig claude;
    claude.name    = "claude";
    claude.apiKey  = "";
    claude.baseUrl = "https://api.anthropic.com/v1";
    claude.model   = "claude-3-5-sonnet-20241022";
    claude.enabled = true;
    cfg_.providers.push_back(claude);

    // Ollama (local)
    ProviderConfig ollama;
    ollama.name    = "ollama";
    ollama.apiKey  = "";
    ollama.baseUrl = "http://127.0.0.1:11434";
    ollama.model   = "qwen2.5-coder:7b";
    ollama.enabled = true;
    cfg_.providers.push_back(ollama);

    cfg_.defaultProvider = "glm";
    cfg_.temperature      = 0.2f;
    cfg_.maxTokens        = 2048;
    cfg_.autoInjectContext = true;
    cfg_.requestTimeoutSec = 60;
}

bool ConfigManager::Load() {
    ResetToDefaults();

    std::string path = GetConfigPath();
    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp) {
        // No config file yet - save defaults
        Save();
        return true;
    }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz <= 0) { fclose(fp); Save(); return true; }

    std::string content(sz, '\0');
    fread(&content[0], 1, sz, fp);
    fclose(fp);

    json_error_t err;
    json_t* root = json_loads(content.c_str(), 0, &err);
    if (!root) {
        Logger::Warn(std::string("Config parse error: ") + err.text);
        Save();
        return false;
    }

    json_t* jdef   = json_object_get(root, "default_provider");
    json_t* jtemp  = json_object_get(root, "temperature");
    json_t* jmax   = json_object_get(root, "max_tokens");
    json_t* jauto  = json_object_get(root, "auto_inject_context");
    json_t* jtime  = json_object_get(root, "request_timeout_sec");
    json_t* jprovs = json_object_get(root, "providers");

    if (jdef  && json_is_string(jdef))  cfg_.defaultProvider  = json_string_value(jdef);
    if (jtemp && json_is_real(jtemp))   cfg_.temperature      = (float)json_real_value(jtemp);
    if (jmax  && json_is_integer(jmax)) cfg_.maxTokens        = (int)json_integer_value(jmax);
    if (jauto && json_is_boolean(jauto)) cfg_.autoInjectContext = json_boolean_value(jauto);
    if (jtime && json_is_integer(jtime)) cfg_.requestTimeoutSec = (int)json_integer_value(jtime);

    if (jprovs && json_is_array(jprovs)) {
        cfg_.providers.clear();
        size_t idx;
        json_t* v;
        json_array_foreach(jprovs, idx, v) {
            ProviderConfig p;
            json_t* jn = json_object_get(v, "name");
            json_t* jk = json_object_get(v, "api_key");
            json_t* jb = json_object_get(v, "base_url");
            json_t* jm = json_object_get(v, "model");
            json_t* je = json_object_get(v, "enabled");
            if (jn) p.name    = json_string_value(jn);
            if (jk) p.apiKey  = json_string_value(jk);
            if (jb) p.baseUrl = json_string_value(jb);
            if (jm) p.model   = json_string_value(jm);
            if (je) p.enabled = json_boolean_value(je);
            if (!p.name.empty()) cfg_.providers.push_back(p);
        }
    }

    json_decref(root);
    return true;
}

bool ConfigManager::Save() {
    std::string path = GetConfigPath();

    json_t* root = json_object();
    json_object_set_new(root, "default_provider", json_string(cfg_.defaultProvider.c_str()));
    json_object_set_new(root, "temperature",       json_real(cfg_.temperature));
    json_object_set_new(root, "max_tokens",        json_integer(cfg_.maxTokens));
    json_object_set_new(root, "auto_inject_context", json_boolean(cfg_.autoInjectContext));
    json_object_set_new(root, "request_timeout_sec", json_integer(cfg_.requestTimeoutSec));

    json_t* arr = json_array();
    for (auto& p : cfg_.providers) {
        json_t* o = json_object();
        json_object_set_new(o, "name",     json_string(p.name.c_str()));
        json_object_set_new(o, "api_key",  json_string(p.apiKey.c_str()));
        json_object_set_new(o, "base_url", json_string(p.baseUrl.c_str()));
        json_object_set_new(o, "model",    json_string(p.model.c_str()));
        json_object_set_new(o, "enabled",  json_boolean(p.enabled));
        json_array_append_new(arr, o);
    }
    json_object_set_new(root, "providers", arr);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump) return false;

    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "wb") != 0 || !fp) {
        free(dump);
        return false;
    }
    fputs(dump, fp);
    fclose(fp);
    free(dump);
    return true;
}
