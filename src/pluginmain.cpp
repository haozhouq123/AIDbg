#include "pluginmain.h"
#include "plugin.h"

// Global variables (required by SDK)
int pluginHandle = 0;
HWND hwndDlg = nullptr;
int hMenu = 0;
int hMenuDisasm = 0;
int hMenuDump = 0;
int hMenuStack = 0;
int hMenuGraph = 0;
int hMenuMemmap = 0;
int hMenuSymmod = 0;

// DllMain - disable thread library calls for performance
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

// pluginit - first function called when plugin is loaded
PLUG_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct) {
    if (!initStruct) return false;
    initStruct->pluginVersion = PLUGIN_VERSION;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, PLUGIN_NAME, _TRUNCATE);
    pluginHandle = initStruct->pluginHandle;
    return pluginInit(initStruct);
}

// plugstop - called when plugin is about to be unloaded
PLUG_EXPORT bool plugstop() {
    pluginStop();
    return true;
}

// plugsetup - called after successful init, for GUI/menu setup
PLUG_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setupStruct) {
    if (!setupStruct) return;
    hwndDlg = setupStruct->hwndDlg;
    hMenu = setupStruct->hMenu;
    hMenuDisasm = setupStruct->hMenuDisasm;
    hMenuDump = setupStruct->hMenuDump;
    hMenuStack = setupStruct->hMenuStack;
    hMenuGraph = setupStruct->hMenuGraph;
    hMenuMemmap = setupStruct->hMenuMemmap;
    hMenuSymmod = setupStruct->hMenuSymmod;
    pluginSetup();
}
