#pragma once
#include "../pluginmain.h"
#include "../utils/ConfigManager.h"

// Modal settings dialog allowing the user to edit the AIDbg config:
// - Default provider dropdown
// - Per-provider: enabled / api_key / base_url / model
// - Temperature / max_tokens / timeout
class SettingsDialog {
public:
    // Show modal dialog. Parent should be the x64dbg main window (hwndDlg).
    static void Show(HWND parent);

private:
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
    static void PopulateProviderList(HWND hDlg);
    static void LoadSelectedProvider(HWND hDlg);
    static void SaveSelectedProvider(HWND hDlg);
    static void ApplyAndClose(HWND hDlg);
};
