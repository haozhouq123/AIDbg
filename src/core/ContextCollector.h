#pragma once
#include "../pluginmain.h"

namespace core {

struct InstructionContext {
    duint                address    = 0;
    std::string          moduleName;
    std::string          label;
    std::string          disasmText;
    std::vector<uint8_t> rawBytes;     // up to 16 bytes
    std::string          registers;
    std::vector<std::string> stackTop; // top 8 stack slots
    std::vector<std::string> nearbyInstructions; // surrounding 4 instructions

    std::string ToMarkdown() const;
};

struct FunctionContext {
    duint                startAddr  = 0;
    duint                endAddr    = 0;
    std::string          moduleName;
    std::vector<std::string> instructions; // disasm lines
    std::vector<std::string> referencedStrings;
    int                  instructionCount = 0;

    std::string ToMarkdown() const;
};

// Collect context for a single instruction
InstructionContext CollectInstruction(duint addr);

// Collect context for an entire function (basic block walk)
FunctionContext CollectFunction(duint addr);

// Format bytes as "48 89 5C 24 10"
std::string FormatBytes(const uint8_t* bytes, size_t n);

// Format a pointer as "0x00007FF6AABBCCDD"
std::string FormatPtr(duint p);

} // namespace core
