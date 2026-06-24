#pragma once
#include "../pluginmain.h"

namespace Logger {

inline void Info(const std::string& msg) {
    _plugin_logprintf("[" PLUGIN_NAME "] %s\n", msg.c_str());
}

inline void Warn(const std::string& msg) {
    _plugin_logprintf("[" PLUGIN_NAME "] WARNING: %s\n", msg.c_str());
}

inline void Error(const std::string& msg) {
    _plugin_logprintf("[" PLUGIN_NAME "] ERROR: %s\n", msg.c_str());
}

// Render HTML directly to log window (x64dbg supports HTML in logs)
inline void Html(const std::string& html) {
    _plugin_lograw_html(html.c_str());
}

// Print a separator line
inline void Separator() {
    _plugin_logputs("[" PLUGIN_NAME "] --------------------------------------------------");
}

// Print a section header
inline void Section(const std::string& title) {
    _plugin_logprintf("[" PLUGIN_NAME "] ====== %s ======\n", title.c_str());
}

} // namespace Logger
