# AIDbg

**AI-powered assistant plugin for x64dbg** — integrate LLMs (GLM-4 / OpenAI / DeepSeek / Claude / Ollama) directly into your reverse engineering workflow.

## Features

- **Ask AI anything** — `ai ask <question>` from the x64dbg command line
- **Explain instruction** — right-click in disassembly view → "AI: Explain this instruction"
- **Vulnerability scan** — right-click → "AI: Vulnerability scan" scans the entire function
- **Multi-provider** — GLM-4.6 / OpenAI GPT-4o / DeepSeek-Coder / Claude 3.5 Sonnet / Ollama (local)
- **Context-aware** — automatically collects registers, stack, bytes, surrounding instructions
- **Settings dialog** — GUI to edit API keys, base URLs, model names

## Quick Start

1. Build the plugin (see [BUILD_VS2022.md](BUILD_VS2022.md) for detailed steps)
2. Copy `AIDbg.dp64` to `<x64dbg>/x64/plugins/`
   (or `AIDbg.dp32` to `<x64dbg>/x32/plugins/` for 32-bit)
3. Launch x64dbg — you should see `[AIDbg] AIDbg v1 loaded` in the log
4. Run `ai config` in the x64dbg command bar to set up your API key
5. Right-click in the disassembly view to access AI features

## Available Commands

| Command | Description |
|---------|-------------|
| `ai ask <question>` | Send a free-form question to the LLM |
| `ai explain [addr]` | Explain the instruction at `addr` (default: current CIP) |
| `ai vuln [addr]` | Scan the function containing `addr` for vulnerabilities |
| `ai config` | Open the settings dialog |
| `ai clear` | Clear the log window |

## Configuration

The config file is stored at:
```
%APPDATA%\x64dbg\AIDbg\config.json
```

Example config:
```json
{
  "default_provider": "glm",
  "providers": [
    {
      "name": "glm",
      "api_key": "YOUR_ZHIPU_API_KEY",
      "base_url": "https://open.bigmodel.cn/api/paas/v4",
      "model": "glm-4.6",
      "enabled": true
    }
  ],
  "temperature": 0.2,
  "max_tokens": 2048,
  "request_timeout_sec": 60
}
```

## Supported Providers

| Provider | Name | Default Model | API Base |
|----------|------|---------------|----------|
| 智谱 GLM  | `glm`     | `glm-4.6`                | `https://open.bigmodel.cn/api/paas/v4` |
| OpenAI    | `openai`  | `gpt-4o-mini`            | `https://api.openai.com/v1` |
| DeepSeek  | `deepseek`| `deepseek-coder`         | `https://api.deepseek.com/v1` |
| Claude    | `claude`  | `claude-3-5-sonnet-20241022` | `https://api.anthropic.com/v1` |
| Ollama    | `ollama`  | `qwen2.5-coder:7b`       | `http://127.0.0.1:11434` |

## Privacy Notes

- AIDbg only sends **disassembly text** and **register values** to the LLM
- It does **NOT** upload memory dumps or file bytes automatically
- For fully offline use, choose the Ollama provider with a local model

## Build Requirements

- Visual Studio 2022 (17.x) with C++ workload
- CMake 3.15+ (bundled with VS 2022)
- Windows SDK 10
- Internet connection on first build (CMake fetches the x64dbg SDK)

See [BUILD_VS2022.md](BUILD_VS2022.md) for step-by-step instructions.

## Architecture

```
┌──────────────────────────────── x64dbg.exe ──────────────────────────────┐
│                                                                          │
│  Disasm View  ──right-click──┐    Registers ──┐    Stack ──┐             │
│                              │                │            │             │
│                              ▼                ▼            ▼             │
│   ┌───────────────────────────────────────────────────────────────────┐  │
│   │                       AIDbg.dp64 (DLL)                            │  │
│   │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐  │  │
│   │  │ Context    │→ │ Prompt     │→ │ LLM Client │→ │ Response   │  │  │
│   │  │ Collector  │  │ Builder    │  │ (WinHTTP)  │  │ Renderer   │  │  │
│   │  └────────────┘  └────────────┘  └────────────┘  └────────────┘  │  │
│   │  ┌────────────┐  ┌────────────┐  ┌────────────┐                   │  │
│   │  │ Provider   │  │ Config     │  │ Commands   │                   │  │
│   │  │ Registry   │  │ Manager    │  │ (ai ...)   │                   │  │
│   │  └────────────┘  └────────────┘  └────────────┘                   │  │
│   └───────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────┼───────────────────────────────────────┘
                                   │ HTTPS
                                   ▼
              ┌────────────────────────────────────────────────┐
              │  Provider Abstraction Layer (WinHTTP)          │
              │  ┌────┐ ┌────┐ ┌────────┐ ┌────────┐ ┌──────┐  │
              │  │GLM │ │GPT │ │DeepSeek│ │Claude  │ │Ollama│  │
              │  └────┘ └────┘ └────────┘ └────────┘ └──────┘  │
              └────────────────────────────────────────────────┘
```

## File Layout

```
AIDbg-source/
├── CMakeLists.txt
├── BUILD_VS2022.md          # Step-by-step build guide
├── README.md
├── cmake/
│   ├── x64dbg.cmake          # x64dbg_plugin() function
│   ├── msvc-static-runtime.cmake
│   └── msvc-configurations.cmake
├── src/
│   ├── pluginmain.h/.cpp     # SDK exports: pluginit/plugstop/plugsetup
│   ├── plugin.h/.cpp         # Plugin entry, menu/command registration
│   ├── utils/
│   │   ├── Logger.h          # Logging helpers
│   │   ├── HttpClient.h/.cpp # WinHTTP async wrapper
│   │   └── ConfigManager.*   # JSON config persistence
│   ├── llm/
│   │   ├── ILLMProvider.h    # Abstract provider interface
│   │   ├── ProviderRegistry.*# Provider manager
│   │   ├── GLMProvider.*     # 智谱 GLM-4
│   │   ├── OpenAIProvider.*  # OpenAI / DeepSeek / Claude
│   │   └── OllamaProvider.*  # Local Ollama
│   ├── core/
│   │   └── ContextCollector.*# Collect disasm/registers/stack context
│   ├── commands/
│   │   └── AICommands.*      # ai ask / ai explain / ai vuln ...
│   └── ui/
│       └── SettingsDialog.*  # Modal settings dialog
└── prompts/
    ├── system_prompt.md
    ├── explain_instruction.md
    ├── detect_vulnerability.md
    ├── generate_patch.md
    └── deobfuscate.md
```

## License

MIT
