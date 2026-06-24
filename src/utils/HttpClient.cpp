#include "HttpClient.h"
#include <mutex>
#include <queue>

namespace http {

// -----------------------------------------------------------------------------
// Internal: GUI thread dispatch via a custom message queue
// -----------------------------------------------------------------------------
constexpr UINT WM_AIDBG_INVOKE = WM_APP + 0xA1D6;

struct InvokeTask {
    std::function<void()> fn;
};

// We register a single hidden window to receive WM_AIDBG_INVOKE messages.
// We also keep a queue protected by mutex (since HWND may not be ready yet).
static std::mutex                g_invokeMutex;
static std::queue<InvokeTask>    g_invokeQueue;
static HWND                      g_invokeHwnd = nullptr;
static WNDPROC                    g_invokeOrigProc = nullptr;

static LRESULT CALLBACK InvokeWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_AIDBG_INVOKE) {
        InvokeTask task;
        {
            std::lock_guard<std::mutex> lk(g_invokeMutex);
            if (!g_invokeQueue.empty()) {
                task = std::move(g_invokeQueue.front());
                g_invokeQueue.pop();
            }
        }
        if (task.fn) {
            try { task.fn(); } catch (...) { /* swallow */ }
        }
        return 0;
    }
    if (g_invokeOrigProc)
        return CallWindowProc(g_invokeOrigProc, hwnd, msg, wp, lp);
    return DefWindowProc(hwnd, msg, wp, lp);
}

// Hook the x64dbg main window on first call
static void EnsureInvokeHook() {
    std::lock_guard<std::mutex> lk(g_invokeMutex);
    if (g_invokeHwnd) return;
    g_invokeHwnd = hwndDlg;
    if (!g_invokeHwnd) return;
    g_invokeOrigProc = (WNDPROC)SetWindowLongPtr(g_invokeHwnd, GWLP_WNDPROC, (LONG_PTR)InvokeWndProc);
}

void RunOnGuiThread(std::function<void()> fn) {
    EnsureInvokeHook();
    {
        std::lock_guard<std::mutex> lk(g_invokeMutex);
        g_invokeQueue.push(InvokeTask{std::move(fn)});
    }
    if (g_invokeHwnd)
        PostMessage(g_invokeHwnd, WM_AIDBG_INVOKE, 0, 0);
}

// -----------------------------------------------------------------------------
// Internal: URL parsing and WinHTTP call
// -----------------------------------------------------------------------------
struct ParsedUrl {
    std::wstring host;
    std::wstring path;
    int          port  = 443;
    bool         https = true;
};

static bool ParseUrl(const std::string& url, ParsedUrl& out) {
    URL_COMPONENTS uc = {sizeof(uc)};
    wchar_t host[256] = {0};
    wchar_t path[2048] = {0};
    uc.lpszHostName     = host;
    uc.dwHostNameLength = 255;
    uc.lpszUrlPath      = path;
    uc.dwUrlPathLength  = 2047;

    std::wstring wurl(url.begin(), url.end());
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc))
        return false;

    out.host   = host;
    out.path   = path;
    out.port   = uc.nPort;
    out.https  = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    return true;
}

static Response DoRequest(const std::string& url,
                          const Headers&     headers,
                          const std::string& body,
                          const wchar_t*     method,
                          int                timeoutSec) {
    Response resp;
    ParsedUrl pu;
    if (!ParseUrl(url, pu)) {
        resp.error = "Invalid URL: " + url;
        return resp;
    }

    HINTERNET hSession = WinHttpOpen(L"AIDbg/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { resp.error = "WinHttpOpen failed"; return resp; }

    WinHttpSetTimeouts(hSession,
        timeoutSec * 1000, timeoutSec * 1000,
        timeoutSec * 1000, timeoutSec * 1000);

    HINTERNET hConnect = WinHttpConnect(hSession, pu.host.c_str(),
        (INTERNET_PORT)pu.port, 0);
    if (!hConnect) {
        resp.error = "WinHttpConnect failed";
        WinHttpCloseHandle(hSession);
        return resp;
    }

    DWORD flags = pu.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method, pu.path.c_str(),
        L"HTTP/1.1", WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        resp.error = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    // Build headers string
    std::wstring hdrs;
    for (auto& [k, v] : headers) {
        std::wstring wk(k.begin(), k.end());
        std::wstring wv(v.begin(), v.end());
        hdrs += wk + L": " + wv + L"\r\n";
    }
    if (body.empty()) {
        // no body
    } else if (hdrs.find(L"Content-Type") == std::wstring::npos) {
        hdrs += L"Content-Type: application/json\r\n";
    }

    BOOL sent = WinHttpSendRequest(hRequest,
        hdrs.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : hdrs.c_str(),
        (DWORD)-1,
        body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str(),
        (DWORD)body.size(),
        (DWORD)body.size(),
        0);
    if (!sent) {
        DWORD e = GetLastError();
        resp.error = "WinHttpSendRequest failed (err=" + std::to_string(e) + ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD e = GetLastError();
        resp.error = "WinHttpReceiveResponse failed (err=" + std::to_string(e) + ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    DWORD statusCode = 0;
    DWORD sz = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &sz,
        WINHTTP_NO_HEADER_INDEX);
    resp.statusCode = (int)statusCode;

    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0) {
        std::vector<char> buf(bytesAvail + 1, 0);
        DWORD bytesRead = 0;
        if (!WinHttpReadData(hRequest, buf.data(), bytesAvail, &bytesRead))
            break;
        resp.body.append(buf.data(), bytesRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return resp;
}

Response Post(const std::string& url, const Headers& headers,
              const std::string& body, int timeoutSec) {
    return DoRequest(url, headers, body, L"POST", timeoutSec);
}

Response Get(const std::string& url, const Headers& headers, int timeoutSec) {
    return DoRequest(url, headers, "", L"GET", timeoutSec);
}

void PostAsync(const std::string& url, const Headers& headers,
               const std::string& body,
               std::function<void(const Response&)> callback,
               int timeoutSec) {
    std::thread([url, headers, body, callback, timeoutSec]() {
        Response resp = Post(url, headers, body, timeoutSec);
        if (callback) callback(resp);
    }).detach();
}

} // namespace http
