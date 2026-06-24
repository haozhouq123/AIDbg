# AIDbg 在 Visual Studio 2022 中的详细编译指南

本文档手把手教你用 VS 2022 把 AIDbg 编译成 `.dp64`（64 位）或 `.dp32`（32 位）插件。

---

## 0. 准备工作

### 0.1 必装组件

| 组件 | 版本要求 | 获取方式 |
|------|---------|---------|
| Visual Studio 2022 | 17.x（Community 免费版即可） | https://visualstudio.microsoft.com/zh-hans/downloads/ |
| C++ 桌面开发工作负载 | 随 VS 安装 | VS Installer 中勾选 |
| Windows 10/11 SDK | 10.0.19041+ | 随 VS 安装 |
| CMake | 3.15+ | VS 2022 自带，无需单独装 |
| Git | 任意版本 | https://git-scm.com/ |

### 0.2 安装 VS 2022 时务必勾选

启动 **Visual Studio Installer** → "修改"，勾选：

- ✅ **使用 C++ 的桌面开发** （Desktop development with C++）
  - 在右侧"安装详细信息"中确保以下子项被勾选：
    - ✅ MSVC v143 - VS 2022 C++ x64/x86 生成工具
    - ✅ Windows 11 SDK（或 Windows 10 SDK 最新版）
    - ✅ 适用于 Windows 的 C++ CMake 工具
    - ✅ 适用于 Windows 的 C++ AddressSanitizer（可选）

> 💡 不要勾选 "使用 C++ 的游戏开发" 或 "使用 C++ 的 Linux 开发"，省时间省空间。

### 0.3 验证环境

打开 **x64 Native Tools Command Prompt for VS 2022**（开始菜单搜），执行：

```cmd
cl
```

应输出 `Microsoft (R) C/C++ Optimizing Compiler Version ... 19.3x` 表示 C++ 编译器就绪。

```cmd
cmake --version
```

应输出 `cmake version 3.2x.x` 表示 CMake 就绪。

---

## 1. 解压源码

把 `AIDbg-source.zip` 解压到一个**没有中文和空格**的路径，例如：

```
D:\Projects\AIDbg-source\
```

解压后目录结构应该是：

```
D:\Projects\AIDbg-source\
├── CMakeLists.txt
├── BUILD_VS2022.md
├── README.md
├── cmake\
├── src\
└── prompts\
```

> ⚠️ **路径不要含中文/空格**，否则 MSVC 编译会出莫名其妙的错误。`C:\Users\张三\Desktop\` 这种就别用了。

---

## 2. 方法 A：用 VS 2022 GUI 编译（推荐新手）

### 2.1 启动 VS 2022

开始菜单 → "Visual Studio 2022" → 启动后选择"**继续但无需代码**"（Continue without code）。

### 2.2 打开 CMake 项目

菜单 **文件** → **打开** → **CMake...**

选择 `D:\Projects\AIDbg-source\CMakeLists.txt`，VS 会开始读取项目。

第一次打开时 VS 会：
1. 检测到 CMake 项目
2. 自动调用 CMake 配置（**会从 SourceForge 下载 x64dbg SDK，约 30 MB**）
3. 生成 `CMakePresets.json`（如果已有则跳过）

**这一步可能需要 1-3 分钟**，看输出窗口（**视图 → 输出 → 显示输出来源: CMake**）能看到进度。

### 2.3 等待 CMake 配置完成

输出窗口最后应该出现：

```
CMake generation started.
CMake generation finished.
```

如果出现错误（红色文字），跳到本文档第 5 节"常见问题"。

### 2.4 选择构建目标架构

VS 顶部工具栏会出现一个 **配置选择器**（一个下拉框），形如：

```
[x64-Debug]    ▼
```

如果看不到，菜单 **项目** → **CMake 设置** → 在右侧面板可以管理。

确保选择 **x64-Debug** 或 **x64-Release**（推荐 Release，体积小、运行快）。

> 📌 编译 `.dp64` 选 x64；编译 `.dp32` 选 x86（Win32）。

### 2.5 生成插件

菜单 **生成** → **全部生成**（Build → Build All）。

或者按 `Ctrl+Shift+B`。

等待编译，输出窗口会显示类似：

```
1>------ 已启动全部生成: 项目: AIDbg (CMake) ------
1>Building Custom Rule D:/Projects/AIDbg-source/CMakeLists.txt
1>pluginmain.cpp
1>plugin.cpp
1>HttpClient.cpp
1>ConfigManager.cpp
1>ProviderRegistry.cpp
1>GLMProvider.cpp
1>OpenAIProvider.cpp
1>OllamaProvider.cpp
1>ContextCollector.cpp
1>AICommands.cpp
1>SettingsDialog.cpp
1>   正在创建库 D:/Projects/AIDbg-source/out/build/x64-Release/AIDbg.lib 和对象 D:/Projects/AIDbg-source/out/build/x64-Release/AIDbg.exp
1>AIDbg.vcxproj -> D:\Projects\AIDbg-source\out\build\x64-Release\AIDbg.dp64
========== 全部生成: 成功 1 个，失败 0 个，跳过 0 个 ==========
```

**关键行**：`AIDbg.vcxproj -> D:\Projects\AIDbg-source\out\build\x64-Release\AIDbg.dp64`

这就是你的插件文件！

### 2.6 定位生成的 .dp64 文件

在 VS **解决方案资源管理器** 中右键 `AIDbg` 项目 → "**在文件资源管理器中打开输出文件夹**"。

或者直接到这个路径找：

```
D:\Projects\AIDbg-source\out\build\x64-Release\AIDbg.dp64
```

同时该目录下应该还有 `prompts\` 子目录（构建后自动复制）。

---

## 3. 方法 B：用命令行编译（推荐自动化/熟练用户）

### 3.1 打开 VS 2022 开发者命令提示符

开始菜单搜索 "**x64 Native Tools Command Prompt for VS 2022**"（编 64 位用）。

或者 "**x86 Native Tools Command Prompt for VS 2022**"（编 32 位用）。

### 3.2 切到源码目录

```cmd
cd /d D:\Projects\AIDbg-source
```

### 3.3 配置项目

**64 位**：
```cmd
cmake -B build64 -A x64 -DCMAKE_BUILD_TYPE=Release
```

**32 位**：
```cmd
cmake -B build32 -A Win32 -DCMAKE_BUILD_TYPE=Release
```

第一次配置会下载 x64dbg SDK（30 MB 左右），输出：

```
-- Fetching x64dbg SDK from SourceForge...
-- Downloading...
-- Extracting...
-- Using x64dbg SDK: .../pluginsdk
-- Building AIDbg as x64 plugin (.dp64)
-- Configuring done
-- Generating done
-- Build files have been written to: D:/Projects/AIDbg-source/build64
```

### 3.4 编译

```cmd
cmake --build build64 --config Release
```

输出尾部应该有：

```
AIDbg.vcxproj -> D:\Projects\AIDbg-source\build64\Release\AIDbg.dp64
```

生成的文件就在：

```
D:\Projects\AIDbg-source\build64\Release\AIDbg.dp64         (64 位)
D:\Projects\AIDbg-source\build32\Release\AIDbg.dp32         (32 位)
```

### 3.5 一行命令同时编 32+64 位（可选）

把以下内容存为 `build_all.cmd` 放到项目根目录：

```cmd
@echo off
echo === Building x64 ===
cmake -B build64 -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build64 --config Release

echo === Building x86 ===
cmake -B build32 -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build32 --config Release

echo === Done ===
echo x64: build64\Release\AIDbg.dp64
echo x86: build32\Release\AIDbg.dp32
pause
```

以后双击即可一键编译。

---

## 4. 方法 C：用 VS 2022 打开 .sln 文件编译

如果方法 A 的"CMake 项目直接打开"在你机器上不顺，可以先用命令行生成 .sln，再用 VS 经典方式打开。

### 4.1 生成 .sln

打开 "x64 Native Tools Command Prompt for VS 2022"，执行：

```cmd
cd /d D:\Projects\AIDbg-source
cmake -B build64 -G "Visual Studio 17 2022" -A x64
```

生成完毕后，`build64\` 目录里会有 `AIDbg.sln`。

### 4.2 打开 .sln 编译

双击 `D:\Projects\AIDbg-source\build64\AIDbg.sln` 用 VS 2022 打开。

在顶部工具栏选择 **Release** + **x64**。

按 `Ctrl+Shift+B` 编译。

输出在 `build64\Release\AIDbg.dp64`。

---

## 5. 常见问题排查

### 5.1 FetchContent 下载 x64dbg SDK 失败

**现象**：CMake 配置阶段卡在 `-- Fetching x64dbg SDK...` 然后报错。

**原因**：SourceForge 国内访问不稳定。

**解决方案**：手动下载 SDK，改用本地路径。

1. 用浏览器（建议挂代理）下载：
   ```
   https://sourceforge.net/projects/x64dbg/files/snapshots/snapshot_2023-06-10_18-05.zip
   ```
2. 解压到例如 `D:\deps\x64dbg\snapshot_2023-06-10_18-05\`，里面应该有 `pluginsdk\` 目录
3. 重新配置 CMake，加参数指向 SDK：
   ```cmd
   cmake -B build64 -A x64 -DX64DBG_SDK_DIR="D:\deps\x64dbg\snapshot_2023-06-10_18-05\pluginsdk" -DAIDBG_USE_LOCAL_SDK=ON
   ```

### 5.2 `winhttp.lib` 链接失败

**现象**：`LINK : fatal error LNK1104: 无法打开文件 winhttp.lib`

**原因**：Windows SDK 未安装或版本太旧。

**解决方案**：在 VS Installer 中勾选 "**Windows 11 SDK**" 或 "**Windows 10 SDK (10.0.19041.0)**" 最新版。

### 5.3 `_CRT_SECURE_NO_WARNINGS` 警告变错误

**现象**：`error C4996: 'fopen': This function or variable may be unsafe...`

**解决方案**：在 CMake 配置时加：
```cmd
cmake -B build64 -A x64 -DCMAKE_CXX_FLAGS="/D_CRT_SECURE_NO_WARNINGS"
```

或在 VS 中：项目属性 → C/C++ → 常规 → 警告等级 → 调低到 **/W1**。

### 5.4 `LNK2038: 检测到"RuntimeLibrary"的不匹配项`

**现象**：链接时一堆 `LNK2038` 错误，提到 `MD_DynamicRelease` 和 `MT_StaticRelease`。

**原因**：x64dbg 用的是 **静态运行时 (/MT)**，但你的项目用了动态运行时 (/MD)。

**解决方案**：项目已经在 `cmake/msvc-static-runtime.cmake` 里强制 /MT。如果仍然报错，手动改：
- 项目属性 → C/C++ → 代码生成 → 运行库 → 选 **多线程 (/MT)**（Release）或 **多线程调试 (/MTd)**（Debug）

### 5.5 找不到 `pluginsdk/bridgemain.h`

**现象**：`fatal error C1083: 无法打开包括文件: "pluginsdk/bridgemain.h"`

**原因**：SDK 路径未找到。

**解决方案**：用方法 5.1 的方式手动指定 `-DX64DBG_SDK_DIR=...`。

### 5.6 编译成功但 x64dbg 加载时崩溃

**排查步骤**：
1. 确认插件文件名是 `AIDbg.dp64`（64 位）或 `AIDbg.dp32`（32 位），**不能**叫 `AIDbg.dll`
2. 确认放在正确的目录：
   - 64 位 x64dbg：`<x64dbg根目录>\x64\plugins\AIDbg.dp64`
   - 32 位 x32dbg：`<x64dbg根目录>\x32\plugins\AIDbg.dp32`
3. 在 x64dbg 日志窗口看是否有 `[AIDbg]` 开头的输出
4. 如果 x64dbg 直接闪退，把插件移走，再次启动 x64dbg 应当正常，确认是插件的问题
5. 用 [PluginDevHelper](https://github.com/x64dbg/PluginDevHelper) 工具辅助调试

### 5.7 VS 提示 "CMake 3.15 required"

**原因**：你的 VS 2022 版本太旧。

**解决方案**：VS Installer → 更新 Visual Studio 2022 到最新版（17.8+）。

---

## 6. 部署到 x64dbg 进行测试

### 6.1 下载 x64dbg

如果你还没有 x64dbg：
- https://x64dbg.com/ → Download
- 或 https://github.com/x64dbg/x64dbg/releases

解压到例如 `D:\Tools\x64dbg\`，结构如下：

```
D:\Tools\x64dbg\
├── x96dbg.exe
├── x32\
│   ├── x32dbg.exe
│   └── plugins\
└── x64\
    ├── x64dbg.exe
    └── plugins\
```

### 6.2 拷贝插件

```
copy D:\Projects\AIDbg-source\build64\Release\AIDbg.dp64 D:\Tools\x64dbg\x64\plugins\
copy D:\Projects\AIDbg-source\build64\Release\prompts D:\Tools\x64dbg\x64\plugins\AIDbg_prompts\ /E /I
```

> 💡 prompts 目录是可选的，当前版本 prompt 已经内置到代码里了。

### 6.3 启动 x64dbg 测试

1. 双击 `D:\Tools\x64dbg\x96dbg.exe` → 选 **x64dbg**
2. 在日志窗口应该看到：
   ```
   [AIDbg] AIDbg v1 loaded (pluginHandle: X)
   [AIDbg] LLM providers initialized. Active: glm
   [AIDbg] Commands registered: ai ask / ai explain / ai vuln / ai config / ai clear
   [AIDbg] Right-click in disassembly view to access AI features.
   ```
3. 在 x64dbg 命令栏（底部）输入：
   ```
   ai config
   ```
   弹出设置对话框，**填入你的 GLM API Key**（去 https://open.bigmodel.cn 注册申请）→ 点 Save。

4. 加载任意一个 exe 进 x64dbg，在反汇编窗口右键 → "**AI: Explain this instruction**"

5. 等几秒，日志窗口会出现 AI 的解释。

### 6.4 测试命令

```
ai ask 什么是 ROP 攻击？
ai explain
ai vuln
```

---

## 7. 调试技巧

### 7.1 在 VS 里附加调试

1. 先启动 x64dbg.exe 并加载 AIDbg 插件
2. VS 菜单 → 调试 → 附加到进程 → 选 `x64dbg.exe`
3. 在你的代码里打断点（比如 `pluginInit` 入口）
4. 在 x64dbg 里通过菜单触发对应操作，VS 会命中断点

### 7.2 用 OutputDebugString 看日志

代码里加 `OutputDebugStringA("[AIDbg] xxx\n")`，用 [DebugView](https://learn.microsoft.com/sysinternals/downloads/debugview) 实时查看。

### 7.3 热重载（避免反复重启 x64dbg）

安装 [PluginDevHelper](https://github.com/x64dbg/PluginDevHelper)：
- 在 x64dbg 菜单 Plugins → PluginDevHelper → Configure
- 添加 AIDbg 项目
- VS 里改完代码按 Ctrl+Shift+B 编译
- PluginDevHelper 会自动卸载旧 .dp64 并加载新的，**无需重启 x64dbg**

---

## 8. 编译产物一览

成功编译后，64 位版本典型产物：

| 文件 | 说明 |
|------|------|
| `AIDbg.dp64` | **核心插件**，拷到 x64dbg 的 `x64\plugins\` |
| `AIDbg.lib` | 静态导入库（不需要部署） |
| `AIDbg.pdb` | 调试符号（保留在开发机，便于 VS 附加调试） |
| `prompts\` | prompt 模板目录（可选部署） |

---

## 9. 完整一键编译脚本（参考）

把以下存为 `D:\Projects\AIDbg-source\build_release.cmd`：

```cmd
@echo off
setlocal

set ROOT=%~dp0
cd /d "%ROOT%"

echo ============================================
echo  AIDbg Build Script
echo  Source: %ROOT%
echo ============================================

REM === Build x64 ===
echo.
echo [1/2] Building x64 (AIDbg.dp64)...
cmake -B build64 -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo x64 CMake configure failed!
    exit /b 1
)
cmake --build build64 --config Release
if errorlevel 1 (
    echo x64 build failed!
    exit /b 1
)

REM === Build x86 ===
echo.
echo [2/2] Building x86 (AIDbg.dp32)...
cmake -B build32 -A Win32 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo x86 CMake configure failed!
    exit /b 1
)
cmake --build build32 --config Release
if errorlevel 1 (
    echo x86 build failed!
    exit /b 1
)

echo.
echo ============================================
echo  Build succeeded!
echo ============================================
echo.
echo  x64 plugin: %ROOT%build64\Release\AIDbg.dp64
echo  x86 plugin: %ROOT%build32\Release\AIDbg.dp32
echo.
endlocal
```

双击运行即可。

---

## 10. 获取帮助

- 卡在某一步？把 **完整输出窗口内容** + **VS 版本** + **CMake 版本** 贴出来
- 插件加载失败？把 x64dbg **日志窗口** 内容贴出来
- 提示 API 错误？检查 `ai config` 里的 API Key 是否填对

Happy hacking!
