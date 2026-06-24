#pragma once
#include "../pluginmain.h"

namespace commands {

// Callback signatures required by _plugin_registercommand
bool cbAi(int argc, char** argv);
bool cbAiAsk(int argc, char** argv);
bool cbAiExplain(int argc, char** argv);
bool cbAiVuln(int argc, char** argv);
bool cbAiConfig(int argc, char** argv);
bool cbAiClear(int argc, char** argv);

// Higher-level actions invoked from menu entries
void ExplainAddress(duint addr);
void ScanVulnerability(duint addr);

} // namespace commands
