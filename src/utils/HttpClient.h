#pragma once
#include "../pluginmain.h"

namespace http {

struct Response {
    int         statusCode = 0;
    std::string body;
    std::string error;
    bool        ok() const { return statusCode >= 200 && statusCode < 300 && error.empty(); }
};

// Header pair (e.g. {"Authorization", "Bearer xxx"})
using Headers = std::vector<std::pair<std::string, std::string>>;

// Synchronous POST (blocks the calling thread; call from a worker thread)
Response Post(const std::string& url,
              const Headers&     headers,
              const std::string& body,
              int                timeoutSec = 60);

// Synchronous GET
Response Get(const std::string& url,
             const Headers&     headers,
             int                timeoutSec = 60);

// Asynchronous POST: runs on a detached thread, calls back on completion.
// Note: callback is NOT on the GUI thread; caller must marshal if needed.
void PostAsync(const std::string& url,
               const Headers&     headers,
               const std::string& body,
               std::function<void(const Response&)> callback,
               int                timeoutSec = 60);

// Run a callable on the GUI thread (x64dbg main window thread).
// Uses PostMessage to hwndDlg with WM_APP+0xAIDBG to dispatch.
void RunOnGuiThread(std::function<void()> fn);

} // namespace http
