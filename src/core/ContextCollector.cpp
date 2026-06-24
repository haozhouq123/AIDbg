#include "ContextCollector.h"
#include <cstdio>

namespace core {

std::string FormatBytes(const uint8_t* bytes, size_t n) {
    std::string s;
    s.reserve(n * 3);
    char buf[4];
    for (size_t i = 0; i < n; i++) {
        if (i > 0) s += ' ';
        sprintf_s(buf, "%02X", bytes[i]);
        s += buf;
    }
    return s;
}

std::string FormatPtr(duint p) {
    char buf[32];
#ifdef _WIN64
    sprintf_s(buf, "0x%016llX", (unsigned long long)p);
#else
    sprintf_s(buf, "0x%08X", (unsigned int)p);
#endif
    return buf;
}

InstructionContext CollectInstruction(duint addr) {
    InstructionContext ctx;
    ctx.address = addr;
    if (!DbgIsDebugging()) return ctx;

    // Read 16 raw bytes
    ctx.rawBytes.resize(16);
    if (!DbgMemRead(addr, ctx.rawBytes.data(), 16))
        ctx.rawBytes.clear();

    // Disassemble at addr
    DISASM_INSTR instr;
    memset(&instr, 0, sizeof(instr));
    if (DbgDisasmAt(addr, &instr)) {
        ctx.disasmText = instr.instruction;
    }

    // Module name
    char modName[MAX_MODULE_SIZE] = {0};
    if (DbgGetModuleAt(addr, modName))
        ctx.moduleName = modName;

    // Label
    char labelText[MAX_LABEL_SIZE] = {0};
    if (DbgGetLabelAt(addr, SEG_DEFAULT, labelText))
        ctx.label = labelText;

    // Registers
    REGDUMP regDump;
    memset(&regDump, 0, sizeof(regDump));
    DbgGetRegDumpEx(&regDump, sizeof(regDump));

    char buf[512];
#ifdef _WIN64
    sprintf_s(buf,
        "RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\n"
        "RSI=%016llX RDI=%016llX RBP=%016llX RSP=%016llX\n"
        "R8 =%016llX R9 =%016llX R10=%016llX R11=%016llX\n"
        "R12=%016llX R13=%016llX R14=%016llX R15=%016llX\n"
        "RIP=%016llX  EFLAGS=%08X",
        (unsigned long long)regDump.regcontext.cax, (unsigned long long)regDump.regcontext.cbx,
        (unsigned long long)regDump.regcontext.ccx, (unsigned long long)regDump.regcontext.cdx,
        (unsigned long long)regDump.regcontext.csi, (unsigned long long)regDump.regcontext.cdi,
        (unsigned long long)regDump.regcontext.cbp, (unsigned long long)regDump.regcontext.csp,
        (unsigned long long)regDump.regcontext.r8,  (unsigned long long)regDump.regcontext.r9,
        (unsigned long long)regDump.regcontext.r10, (unsigned long long)regDump.regcontext.r11,
        (unsigned long long)regDump.regcontext.r12, (unsigned long long)regDump.regcontext.r13,
        (unsigned long long)regDump.regcontext.r14, (unsigned long long)regDump.regcontext.r15,
        (unsigned long long)regDump.regcontext.cip, (unsigned int)regDump.regcontext.eflags);
#else
    sprintf_s(buf,
        "EAX=%08X EBX=%08X ECX=%08X EDX=%08X\n"
        "ESI=%08X EDI=%08X EBP=%08X ESP=%08X\n"
        "EIP=%08X  EFLAGS=%08X",
        (unsigned int)regDump.regcontext.cax, (unsigned int)regDump.regcontext.cbx,
        (unsigned int)regDump.regcontext.ccx, (unsigned int)regDump.regcontext.cdx,
        (unsigned int)regDump.regcontext.csi, (unsigned int)regDump.regcontext.cdi,
        (unsigned int)regDump.regcontext.cbp, (unsigned int)regDump.regcontext.csp,
        (unsigned int)regDump.regcontext.cip, (unsigned int)regDump.regcontext.eflags);
#endif
    ctx.registers = buf;

    // Stack top 8 entries
    duint sp = regDump.regcontext.csp;
    for (int i = 0; i < 8; i++) {
        duint v = 0;
        if (DbgMemRead(sp + i * sizeof(duint), &v, sizeof(v))) {
            ctx.stackTop.push_back(FormatPtr(v));
        }
    }

    // Nearby instructions (walk backwards/forwards using 1-byte steps until disasm succeeds)
    auto tryDisasm = [](duint a, std::string& out) -> bool {
        DISASM_INSTR di;
        memset(&di, 0, sizeof(di));
        if (DbgDisasmAt(a, &di) && di.instr_size > 0) {
            char buf2[128];
            sprintf_s(buf2, "%s  %s", FormatPtr(a).c_str(), di.instruction);
            out = buf2;
            return true;
        }
        return false;
    };

    // Walk forward for "next" instructions
    duint p = addr;
    for (int i = 0; i < 4 && p < addr + 64; ) {
        DISASM_INSTR di; memset(&di, 0, sizeof(di));
        if (DbgDisasmAt(p, &di) && di.instr_size > 0) {
            if (p != addr) {
                std::string s;
                if (tryDisasm(p, s)) { ctx.nearbyInstructions.push_back(s); i++; }
            }
            p += di.instr_size;
        } else {
            p++;
        }
    }

    return ctx;
}

std::string InstructionContext::ToMarkdown() const {
    std::string s = "## Instruction Context\n\n";
    s += "- **Address**: `" + FormatPtr(address) + "`\n";
    if (!moduleName.empty())
        s += "- **Module**: `" + moduleName + "`\n";
    if (!label.empty())
        s += "- **Label**: `" + label + "`\n";
    if (!disasmText.empty())
        s += "- **Disasm**: `" + disasmText + "`\n";
    if (!rawBytes.empty())
        s += "- **Bytes**: `" + FormatBytes(rawBytes.data(), rawBytes.size()) + "`\n";
    s += "\n### Registers\n```\n" + registers + "\n```\n";
    if (!stackTop.empty()) {
        s += "\n### Stack (top " + std::to_string(stackTop.size()) + " slots)\n```\n";
        for (size_t i = 0; i < stackTop.size(); i++)
            s += "[rsp+" + std::to_string(i * sizeof(duint)) + "] = " + stackTop[i] + "\n";
        s += "```\n";
    }
    if (!nearbyInstructions.empty()) {
        s += "\n### Surrounding instructions\n```\n";
        for (auto& ni : nearbyInstructions) s += ni + "\n";
        s += "```\n";
    }
    return s;
}

FunctionContext CollectFunction(duint addr) {
    FunctionContext fc;
    fc.startAddr = addr;

    // Try to get the function range
    duint funcStart = 0, funcEnd = 0;
    if (DbgFunctionGet(addr, &funcStart, &funcEnd)) {
        fc.startAddr = funcStart;
        fc.endAddr   = funcEnd;
    } else {
        // Fallback: assume 200 instructions max starting from addr
        fc.startAddr = addr;
        fc.endAddr   = addr + 200 * 8;
    }

    char modName[MAX_MODULE_SIZE] = {0};
    if (DbgGetModuleAt(fc.startAddr, modName))
        fc.moduleName = modName;

    // Walk through the function and collect instructions
    duint p = fc.startAddr;
    int maxInstructions = 500;  // safety limit
    while (p < fc.endAddr && maxInstructions-- > 0) {
        DISASM_INSTR di;
        memset(&di, 0, sizeof(di));
        if (!DbgDisasmAt(p, &di) || di.instr_size <= 0) {
            p++;
            continue;
        }
        char buf[256];
        sprintf_s(buf, "%s  %s", FormatPtr(p).c_str(), di.instruction);
        fc.instructions.push_back(buf);
        fc.instructionCount++;

        // Look for memory references that might be strings
        for (int i = 0; i < di.argcount; i++) {
            const auto& arg = di.arg[i];
            if (arg.type == arg_memory && arg.value != 0) {
                char strBuf[128] = {0};
                if (DbgGetStringAt(arg.value, strBuf) && strBuf[0]) {
                    fc.referencedStrings.push_back(
                        std::string("\"") + strBuf + "\" @ " + FormatPtr(arg.value));
                }
            }
        }

        p += di.instr_size;
    }

    return fc;
}

std::string FunctionContext::ToMarkdown() const {
    std::string s = "## Function Context\n\n";
    s += "- **Start**: `" + FormatPtr(startAddr) + "`\n";
    s += "- **End**:   `" + FormatPtr(endAddr) + "`\n";
    if (!moduleName.empty())
        s += "- **Module**: `" + moduleName + "`\n";
    s += "- **Instructions**: " + std::to_string(instructionCount) + "\n";

    if (!referencedStrings.empty()) {
        s += "\n### Referenced strings\n```\n";
        for (auto& str : referencedStrings) s += str + "\n";
        s += "```\n";
    }

    s += "\n### Disassembly\n```\n";
    int n = 0;
    for (auto& ins : instructions) {
        s += ins + "\n";
        if (++n >= 200) {
            s += "... (truncated, " + std::to_string(instructions.size() - 200) + " more)\n";
            break;
        }
    }
    s += "```\n";
    return s;
}

} // namespace core
