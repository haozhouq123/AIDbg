#pragma once
#include "pluginmain.h"

// Plugin lifecycle functions
bool pluginInit(PLUG_INITSTRUCT* initStruct);
void pluginStop();
void pluginSetup();

// Menu entry IDs (must be unique within the plugin)
enum MenuEntryID {
    MENU_MAIN_TOGGLE_LOG   = 1,
    MENU_MAIN_CLEAR_LOG    = 2,
    MENU_MAIN_SETTINGS     = 3,
    MENU_MAIN_ABOUT        = 4,
    MENU_DISASM_EXPLAIN    = 100,
    MENU_DISASM_VULN_SCAN  = 101,
    MENU_DISASM_ASK        = 102,
};
