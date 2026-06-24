#include "plugin.h"
#include "utils/Logger.h"
#include "utils/ConfigManager.h"
#include "llm/ProviderRegistry.h"
#include "commands/AICommands.h"
#include "ui/SettingsDialog.h"

// Helper: get the currently selected address in the disassembly view
static duint GetDisasmSelection() {
    SELECTIONDATA sel = {0};
    if (GuiSelectionGet(GUI_DISASSEMBLY, &sel) && sel.start)
        return sel.start;
    return DbgEval("cip");
}

bool pluginInit(PLUG_INITSTRUCT* initStruct) {
    dprintf("AIDbg v%d loaded (pluginHandle: %d)\n", PLUGIN_VERSION, pluginHandle);

    // Load configuration from disk
    ConfigManager::Instance().Load();

    // Register commands (case-insensitive, comma-separated args)
    _plugin_registercommand(pluginHandle, "ai",          commands::cbAi,        true);
    _plugin_registercommand(pluginHandle, "ai ask",      commands::cbAiAsk,     true);
    _plugin_registercommand(pluginHandle, "ai explain",  commands::cbAiExplain, true);
    _plugin_registercommand(pluginHandle, "ai vuln",     commands::cbAiVuln,    true);
    _plugin_registercommand(pluginHandle, "ai config",   commands::cbAiConfig,  true);
    _plugin_registercommand(pluginHandle, "ai clear",    commands::cbAiClear,   true);

    // Initialize LLM providers from config
    ProviderRegistry::Instance().Initialize();

    dputs("Commands registered: ai ask / ai explain / ai vuln / ai config / ai clear");
    return true;
}

void pluginStop() {
    dprintf("AIDbg unloading (pluginHandle: %d)\n", pluginHandle);

    _plugin_unregistercommand(pluginHandle, "ai");
    _plugin_unregistercommand(pluginHandle, "ai ask");
    _plugin_unregistercommand(pluginHandle, "ai explain");
    _plugin_unregistercommand(pluginHandle, "ai vuln");
    _plugin_unregistercommand(pluginHandle, "ai config");
    _plugin_unregistercommand(pluginHandle, "ai clear");

    // Save config
    ConfigManager::Instance().Save();
}

void pluginSetup() {
    // Build plugin submenu under "Plugins" -> "AI Assistant"
    int hAiMenu = _plugin_menuadd(hMenu, "AI Assistant");
    _plugin_menuaddentry(hAiMenu, MENU_MAIN_TOGGLE_LOG,  "Show usage hint");
    _plugin_menuaddentry(hAiMenu, MENU_MAIN_CLEAR_LOG,   "Clear log");
    _plugin_menuaddseparator(hAiMenu);
    _plugin_menuaddentry(hAiMenu, MENU_MAIN_SETTINGS,    "Settings...");
    _plugin_menuaddentry(hAiMenu, MENU_MAIN_ABOUT,       "About AIDbg");

    // Right-click menu in the disassembly view
    _plugin_menuaddentry(hMenuDisasm, MENU_DISASM_EXPLAIN,   "AI: Explain this instruction");
    _plugin_menuaddentry(hMenuDisasm, MENU_DISASM_VULN_SCAN, "AI: Vulnerability scan");
    _plugin_menuaddentry(hMenuDisasm, MENU_DISASM_ASK,       "AI: Ask about this address");

    // Register callback for menu entries
    _plugin_registercallback(pluginHandle, CB_MENUENTRY, [](CBTYPE, void* info) {
        auto* cb = (PLUG_CB_MENUENTRY*)info;
        switch (cb->hEntry) {
            case MENU_MAIN_TOGGLE_LOG:
                _plugin_logputs("[" PLUGIN_NAME "] Usage:\n"
                                "  ai ask <question>        - ask the LLM anything\n"
                                "  ai explain [addr]        - explain an instruction\n"
                                "  ai vuln [addr]           - scan a function for vulnerabilities\n"
                                "  ai config                - open settings dialog\n"
                                "  ai clear                 - clear log\n"
                                "Right-click in the disassembly view for AI menu options.");
                break;
            case MENU_MAIN_CLEAR_LOG:
                DbgCmdExec("clear");
                break;
            case MENU_MAIN_SETTINGS:
                SettingsDialog::Show(hwndDlg);
                break;
            case MENU_MAIN_ABOUT:
                MessageBoxA(hwndDlg,
                    "AIDbg v0.1.0\n"
                    "AI-powered assistant for x64dbg\n\n"
                    "Supports: GLM-4 / OpenAI / DeepSeek / Ollama\n\n"
                    "Configure API keys via Settings dialog or\n"
                    "edit: %APPDATA%\\x64dbg\\AIDbg\\config.json",
                    "About AIDbg", MB_OK | MB_ICONINFORMATION);
                break;
            case MENU_DISASM_EXPLAIN: {
                commands::ExplainAddress(GetDisasmSelection());
                break;
            }
            case MENU_DISASM_VULN_SCAN: {
                commands::ScanVulnerability(GetDisasmSelection());
                break;
            }
            case MENU_DISASM_ASK: {
                duint addr = GetDisasmSelection();
                char buf[64];
                sprintf_s(buf, "ai explain 0x%p", addr);
                DbgCmdExec(buf);
                break;
            }
        }
    });

    dputs("Right-click in disassembly view to access AI features.");
}
