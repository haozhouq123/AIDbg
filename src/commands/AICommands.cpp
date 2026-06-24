#include "AICommands.h"
#include "../utils/Logger.h"
#include "../utils/ConfigManager.h"
#include "../utils/HttpClient.h"
#include "../llm/ProviderRegistry.h"
#include "../core/ContextCollector.h"
#include "../ui/SettingsDialog.h"

#include <cstdio>

namespace commands {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const char* kSystemPrompt =
    "You are AIDbg, an AI assistant embedded inside the x64dbg debugger.\n"
    "You help reverse engineers understand disassembly, identify vulnerabilities,\n"
    "and generate patches. Be concise and technical. Use Intel syntax.\n"
    "When showing code, use markdown code blocks. Cite addresses as 0x...\n"
    "If the user provides register/stack context, use it to inform your answer.";

static void PrintResponse(const llm::Response& r, const std::string& title) {
    Logger::Separator();
    Logger::Section(title);
    if (!r.ok()) {
        Logger::Error("LLM error: " + r.error);
        Logger::Separator();
        return;
    }
    // Render as plain text via _plugin_logprintf (HTML escaping done by x64dbg)
    // For better formatting, split into lines
    std::string text = r.content;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = (nl == std::string::npos)
            ? text.substr(pos)
            : text.substr(pos, nl - pos);
        _plugin_logprintf("%s\n", line.c_str());
        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
    char stats[128];
    sprintf_s(stats, "[tokens in=%d, out=%d]", r.inputTokens, r.outputTokens);
    Logger::Info(stats);
    Logger::Separator();
}

static llm::Request MakeRequest(const std::string& userText) {
    llm::Request req;
    req.systemPrompt = kSystemPrompt;
    req.temperature  = ConfigManager::Instance().Get().temperature;
    req.maxTokens    = ConfigManager::Instance().Get().maxTokens;
    req.timeoutSec   = ConfigManager::Instance().Get().requestTimeoutSec;
    req.messages.push_back({"user", userText});
    return req;
}

static llm::IProvider* GetProviderOrWarn() {
    auto* p = llm::ProviderRegistry::Instance().GetActive();
    if (!p) {
        Logger::Error("No LLM provider available. Run 'ai config' to set up.");
    }
    return p;
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

bool cbAi(int argc, char** argv) {
    if (argc < 2) {
        dputs("Usage: ai <ask|explain|vuln|config|clear> [args]");
        dputs("  ai ask <question>     - ask anything to the LLM");
        dputs("  ai explain [addr]     - explain instruction at addr (default: cip)");
        dputs("  ai vuln [addr]        - scan function for vulnerabilities");
        dputs("  ai config             - open settings dialog");
        dputs("  ai clear              - clear log");
        return true;
    }
    // Route to subcommand
    std::string sub = argv[1];
    if (sub == "ask")     return cbAiAsk(argc - 1, argv + 1);
    if (sub == "explain") return cbAiExplain(argc - 1, argv + 1);
    if (sub == "vuln")    return cbAiVuln(argc - 1, argv + 1);
    if (sub == "config")  return cbAiConfig(argc - 1, argv + 1);
    if (sub == "clear")   return cbAiClear(argc - 1, argv + 1);
    dprintf("Unknown subcommand: %s\n", sub.c_str());
    return false;
}

bool cbAiAsk(int argc, char** argv) {
    if (argc < 2) {
        dputs("Usage: ai ask <question>");
        return false;
    }
    // Concatenate all args (in case the question was split by spaces)
    std::string question;
    for (int i = 1; i < argc; i++) {
        if (i > 1) question += ' ';
        question += argv[i];
    }

    auto* provider = GetProviderOrWarn();
    if (!provider) return false;

    Logger::Section("Asking: " + question);
    dputs("[AIDbg] Sending request (this may take a few seconds)...");

    auto req = MakeRequest(question);
    provider->CompleteAsync(req, [](const llm::Response& r) {
        http::RunOnGuiThread([r]() {
            PrintResponse(r, "AI Response");
        });
    });
    return true;
}

bool cbAiExplain(int argc, char** argv) {
    duint addr = (argc >= 2) ? DbgEval(argv[1]) : DbgEval("cip");
    ExplainAddress(addr);
    return true;
}

bool cbAiVuln(int argc, char** argv) {
    duint addr = (argc >= 2) ? DbgEval(argv[1]) : DbgEval("cip");
    ScanVulnerability(addr);
    return true;
}

bool cbAiConfig(int /*argc*/, char** /*argv*/) {
    SettingsDialog::Show(hwndDlg);
    return true;
}

bool cbAiClear(int /*argc*/, char** /*argv*/) {
    DbgCmdExec("clear");
    return true;
}

// ---------------------------------------------------------------------------
// Menu actions
// ---------------------------------------------------------------------------

void ExplainAddress(duint addr) {
    if (!DbgIsDebugging()) {
        dputs("No process is being debugged.");
        return;
    }
    auto* provider = GetProviderOrWarn();
    if (!provider) return;

    auto ctx = core::CollectInstruction(addr);
    char header[128];
    sprintf_s(header, "Explaining instruction at 0x%p", (void*)addr);
    Logger::Section(header);

    std::string prompt =
        "Explain the following x64dbg instruction context. Cover:\n"
        "1. What this instruction does\n"
        "2. Side effects (registers/flags/memory modified)\n"
        "3. The likely intent in the broader function\n"
        "4. Any interesting observations about the surrounding code\n\n"
        + ctx.ToMarkdown() +
        "\n\nProvide a concise technical explanation.";

    auto req = MakeRequest(prompt);
    provider->CompleteAsync(req, [addr](const llm::Response& r) {
        http::RunOnGuiThread([r, addr]() {
            char title[128];
            sprintf_s(title, "Explanation @ 0x%p", (void*)addr);
            PrintResponse(r, title);
        });
    });

    dputs("[AIDbg] Request sent. Response will appear in the log shortly.");
}

void ScanVulnerability(duint addr) {
    if (!DbgIsDebugging()) {
        dputs("No process is being debugged.");
        return;
    }
    auto* provider = GetProviderOrWarn();
    if (!provider) return;

    Logger::Section("Scanning for vulnerabilities...");
    dputs("[AIDbg] Collecting function context (this may take a moment)...");

    // Run collection on worker thread to avoid blocking GUI
    std::thread([addr]() {
        auto fctx = core::CollectFunction(addr);

        std::string prompt =
            "Analyze the following function for potential security vulnerabilities.\n"
            "Look for:\n"
            "- Buffer overflows (stack/heap)\n"
            "- Integer overflows / underflows\n"
            "- Format string vulnerabilities\n"
            "- Use-after-free / double-free\n"
            "- Insecure API usage (strcpy, sprintf, gets, etc.)\n"
            "- Logic bugs in boundary checks\n"
            "- Anti-debugging / anti-analysis tricks\n\n"
            "For each finding, provide:\n"
            "1. Severity (Critical/High/Medium/Low/Info)\n"
            "2. Address of the vulnerable instruction\n"
            "3. Description of the issue\n"
            "4. Recommended mitigation\n\n"
            + fctx.ToMarkdown();

        auto req = MakeRequest(prompt);
        req.maxTokens = 4096;  // allow longer responses for vuln reports

        auto* p = llm::ProviderRegistry::Instance().GetActive();
        if (!p) return;

        p->CompleteAsync(req, [](const llm::Response& r) {
            http::RunOnGuiThread([r]() {
                PrintResponse(r, "Vulnerability Report");
            });
        });
    }).detach();

    dputs("[AIDbg] Function scan in progress. Result will appear in the log.");
}

} // namespace commands
