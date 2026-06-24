#include "SettingsDialog.h"
#include "../utils/Logger.h"
#include "../llm/ProviderRegistry.h"

#include <commctrl.h>

namespace {

constexpr int IDC_PROVIDER_LIST   = 1001;
constexpr int IDC_API_KEY         = 1002;
constexpr int IDC_BASE_URL        = 1003;
constexpr int IDC_MODEL           = 1004;
constexpr int IDC_ENABLED         = 1005;
constexpr int IDC_DEFAULT_PROV    = 1006;
constexpr int IDC_TEMPERATURE     = 1007;
constexpr int IDC_MAX_TOKENS      = 1008;
constexpr int IDC_TIMEOUT         = 1009;
constexpr int IDC_BTN_SAVE        = 1010;
constexpr int IDC_BTN_CLOSE       = 1011;
constexpr int IDC_BTN_RESET       = 1012;

// Tracks the currently-selected provider index in the dialog
int g_currentProviderIdx = -1;

void AddControl(HWND parent, const wchar_t* cls, const wchar_t* text,
                DWORD style, int x, int y, int w, int h, int id) {
    HWND h = CreateWindowW(cls, text ? text : L"",
        WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, parent, (HMENU)(INT_PTR)id,
        (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE), NULL);
    if (h) SendMessage(h, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

void AddLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
    AddControl(parent, L"STATIC", text, SS_LEFT, x, y, w, h, 0);
}

} // namespace

void SettingsDialog::PopulateProviderList(HWND hDlg) {
    HWND hList = GetDlgItem(hDlg, IDC_PROVIDER_LIST);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);

    auto& cfg = ConfigManager::Instance().Get();
    for (size_t i = 0; i < cfg.providers.size(); i++) {
        std::wstring ws(cfg.providers[i].name.begin(), cfg.providers[i].name.end());
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)ws.c_str());
    }

    HWND hDefault = GetDlgItem(hDlg, IDC_DEFAULT_PROV);
    SendMessage(hDefault, CB_RESETCONTENT, 0, 0);
    for (auto& p : cfg.providers) {
        std::wstring ws(p.name.begin(), p.name.end());
        SendMessageW(hDefault, CB_ADDSTRING, 0, (LPARAM)ws.c_str());
    }
    std::wstring wdef(cfg.defaultProvider.begin(), cfg.defaultProvider.end());
    SendMessageW(hDefault, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)wdef.c_str());

    // Select first provider by default
    if (!cfg.providers.empty()) {
        SendMessage(hList, LB_SETCURSEL, 0, 0);
        g_currentProviderIdx = 0;
        LoadSelectedProvider(hDlg);
    }
}

void SettingsDialog::LoadSelectedProvider(HWND hDlg) {
    HWND hList = GetDlgItem(hDlg, IDC_PROVIDER_LIST);
    int idx = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR) return;
    g_currentProviderIdx = idx;

    auto& cfg = ConfigManager::Instance().Get();
    if (idx < 0 || idx >= (int)cfg.providers.size()) return;

    auto& p = cfg.providers[idx];
    SetDlgItemTextA(hDlg, IDC_API_KEY,  p.apiKey.c_str());
    SetDlgItemTextA(hDlg, IDC_BASE_URL, p.baseUrl.c_str());
    SetDlgItemTextA(hDlg, IDC_MODEL,    p.model.c_str());
    CheckDlgButton(hDlg, IDC_ENABLED, p.enabled ? BST_CHECKED : BST_UNCHECKED);

    char buf[32];
    sprintf_s(buf, "%.2f", cfg.temperature);
    SetDlgItemTextA(hDlg, IDC_TEMPERATURE, buf);
    sprintf_s(buf, "%d", cfg.maxTokens);
    SetDlgItemTextA(hDlg, IDC_MAX_TOKENS, buf);
    sprintf_s(buf, "%d", cfg.requestTimeoutSec);
    SetDlgItemTextA(hDlg, IDC_TIMEOUT, buf);
}

void SettingsDialog::SaveSelectedProvider(HWND hDlg) {
    auto& cfg = ConfigManager::Instance().Get();
    if (g_currentProviderIdx < 0 || g_currentProviderIdx >= (int)cfg.providers.size()) return;
    auto& p = cfg.providers[g_currentProviderIdx];

    char buf[1024];
    GetDlgItemTextA(hDlg, IDC_API_KEY, buf, 1024);  p.apiKey  = buf;
    GetDlgItemTextA(hDlg, IDC_BASE_URL, buf, 1024); p.baseUrl = buf;
    GetDlgItemTextA(hDlg, IDC_MODEL, buf, 1024);    p.model   = buf;
    p.enabled = IsDlgButtonChecked(hDlg, IDC_ENABLED) == BST_CHECKED;

    GetDlgItemTextA(hDlg, IDC_TEMPERATURE, buf, 32);
    cfg.temperature = (float)atof(buf);
    GetDlgItemTextA(hDlg, IDC_MAX_TOKENS, buf, 32);
    cfg.maxTokens = atoi(buf);
    GetDlgItemTextA(hDlg, IDC_TIMEOUT, buf, 32);
    cfg.requestTimeoutSec = atoi(buf);

    // Default provider dropdown
    HWND hDef = GetDlgItem(hDlg, IDC_DEFAULT_PROV);
    int defIdx = (int)SendMessage(hDef, CB_GETCURSEL, 0, 0);
    if (defIdx != CB_ERR && defIdx < (int)cfg.providers.size())
        cfg.defaultProvider = cfg.providers[defIdx].name;
}

void SettingsDialog::ApplyAndClose(HWND hDlg) {
    SaveSelectedProvider(hDlg);
    ConfigManager::Instance().Save();
    llm::ProviderRegistry::Instance().Initialize();
    Logger::Info("Settings saved. Active provider: " +
                 llm::ProviderRegistry::Instance().GetActiveName());
    EndDialog(hDlg, IDOK);
}

INT_PTR CALLBACK SettingsDialog::DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG: {
        // Window caption
        SetWindowTextW(hDlg, L"AIDbg Settings");

        // Provider list
        AddLabel(hDlg, L"Providers:", 10, 10, 100, 18);
        AddControl(hDlg, L"LISTBOX", NULL,
            WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            10, 30, 150, 120, IDC_PROVIDER_LIST);

        // Provider fields
        AddLabel(hDlg, L"API Key:",  180, 10, 100, 18);
        AddControl(hDlg, L"EDIT",    L"",
            WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
            180, 30, 280, 22, IDC_API_KEY);

        AddLabel(hDlg, L"Base URL:", 180, 60, 100, 18);
        AddControl(hDlg, L"EDIT",    L"",
            WS_BORDER | ES_AUTOHSCROLL,
            180, 80, 280, 22, IDC_BASE_URL);

        AddLabel(hDlg, L"Model:",    180, 110, 100, 18);
        AddControl(hDlg, L"EDIT",    L"",
            WS_BORDER | ES_AUTOHSCROLL,
            180, 130, 280, 22, IDC_MODEL);

        AddControl(hDlg, L"BUTTON",  L"Enabled",
            BS_AUTOCHECKBOX,
            180, 160, 100, 22, IDC_ENABLED);

        // Global settings
        AddLabel(hDlg, L"Default provider:", 10, 165, 110, 18);
        AddControl(hDlg, L"COMBOBOX", NULL,
            CBS_DROPDOWNLIST | WS_VSCROLL,
            10, 185, 150, 200, IDC_DEFAULT_PROV);

        AddLabel(hDlg, L"Temperature:",  180, 195, 80, 18);
        AddControl(hDlg, L"EDIT",        L"0.2",
            WS_BORDER | ES_AUTOHSCROLL,
            270, 195, 60, 22, IDC_TEMPERATURE);

        AddLabel(hDlg, L"Max tokens:",   340, 195, 70, 18);
        AddControl(hDlg, L"EDIT",        L"2048",
            WS_BORDER | ES_AUTOHSCROLL,
            410, 195, 60, 22, IDC_MAX_TOKENS);

        AddLabel(hDlg, L"Timeout (s):",  180, 225, 80, 18);
        AddControl(hDlg, L"EDIT",        L"60",
            WS_BORDER | ES_AUTOHSCROLL,
            270, 225, 60, 22, IDC_TIMEOUT);

        // Buttons
        AddControl(hDlg, L"BUTTON", L"Save",    WS_TABSTOP, 280, 270, 80, 26, IDC_BTN_SAVE);
        AddControl(hDlg, L"BUTTON", L"Reset",   WS_TABSTOP, 370, 270, 80, 26, IDC_BTN_RESET);
        AddControl(hDlg, L"BUTTON", L"Close",   WS_TABSTOP, 460, 270, 80, 26, IDC_BTN_CLOSE);

        PopulateProviderList(hDlg);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_PROVIDER_LIST:
            if (HIWORD(wp) == LBN_SELCHANGE)
                LoadSelectedProvider(hDlg);
            break;
        case IDC_BTN_SAVE:
            ApplyAndClose(hDlg);
            break;
        case IDC_BTN_RESET:
            ConfigManager::Instance().ResetToDefaults();
            ConfigManager::Instance().Save();
            PopulateProviderList(hDlg);
            MessageBoxA(hDlg, "Config reset to defaults.", "AIDbg", MB_OK);
            break;
        case IDC_BTN_CLOSE:
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        break;
    }
    return FALSE;
}

void SettingsDialog::Show(HWND parent) {
    // Use a dialog resource ID 0 (we create controls manually in WM_INITDIALOG)
    // We need a registered window class. Use DialogBox with a dummy template.
    // Simpler: create a popup window and handle WM_CREATE -> WM_INITDIALOG manually.

    // Use CreateDialog with a minimal in-memory template
    static unsigned char tmpl[] = {
        // DLGTEMPLATE
        0x00, 0x00, // style low (will be overridden)
        0x80, 0xC0, // style high: WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER
        0x00, 0x00, // exStyle
        0x00,       // cdit
        0x00, 0x00, // x, y
        0x90, 0x00, // cx
        0xB0, 0x00, // cy
        0x00, 0x00  // menu, class, title (empty)
    };

    // Wait, building a proper DLGTEMPLATE in memory is tricky.
    // Use the simpler approach: a normal WS_POPUP window with our own message loop.
    // But that's non-modal. For modal behavior, just use DialogBoxIndirectParam.
    // The above template isn't quite right - use a proper one below.

    struct {
        DLGTEMPLATE tmpl;
        WORD menu;
        WORD cls;
        WORD title;
    } dt = {
        { WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER,
          0, 0, 0, 0, 0, 560, 320 },
        0, 0, 0
    };

    DialogBoxIndirectParamW(GetModuleHandleW(NULL),
        (LPCDLGTEMPLATEW)&dt, parent,
        (DLGPROC)DlgProc, 0);
}
