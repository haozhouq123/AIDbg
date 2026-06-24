# 用 GitHub Actions 编译 AIDbg 完整指南

> 不需要本地装 Visual Studio，不需要 CMake，全在云端编译。免费，每次约 5 分钟。

---

## 总览

整个流程分为 4 步：

1. **注册 GitHub 账号**（如果没有的话）
2. **在 GitHub 上创建仓库** 并上传 AIDbg 源码
3. **触发 GitHub Actions 自动编译**（推送代码即触发）
4. **下载编译产物** `AIDbg.dp64` / `AIDbg.dp32`

---

## 步骤 1：注册 GitHub 账号（如已有跳过）

访问 https://github.com/signup

- 用户名：自定义（建议英文）
- 邮箱：建议用国内可访问的（Gmail/Outlook/QQ 邮箱均可）
- 密码：8 位以上，含字母数字

注册完成后登录 GitHub。

---

## 步骤 2：创建 GitHub 仓库

### 2.1 创建新仓库

1. 登录 GitHub，点击右上角 **"+"** → **"New repository"**

2. 填写仓库信息：
   - **Repository name**: `AIDbg`
   - **Description** (可选): `AI-powered assistant plugin for x64dbg`
   - **可见性**: 选 **Public**（公开，免费用户只能用公开仓库的无限 Actions）
     - 如果你有 GitHub Pro ($4/月)，可以选 Private
   - **Initialize this repository with**: 全部**不勾选**
     - 不要勾 "Add a README file"
     - 不要勾 "Add .gitignore"
     - 不要勾 "Choose a license"

3. 点击 **"Create repository"**

4. 创建后 GitHub 会显示一个空仓库页面，**记下仓库地址**，形如：
   ```
   https://github.com/your-username/AIDbg.git
   ```

### 2.2 选择上传方式

GitHub 提供两种上传方式，选其一即可：

---

#### 方式 A：网页直接上传（最简单，无需装 Git）

1. 解压 `AIDbg-source.zip` 到本地某目录

2. 回到 GitHub 仓库页面，点击 **"uploading an existing file"** 链接

3. **关键**：因为 `.github` 是隐藏目录，**网页上传会跳过它**！所以必须分两批上传：

   **第一批**：上传所有非隐藏文件
   - 把 AIDbg-source 目录下**除了 .github 和 .gitignore 和 .gitattributes 之外**的所有文件拖到浏览器上传框
   - 等待上传完成
   - 在 "Commit changes" 对话框：
     - Commit message: `Initial commit: AIDbg plugin source`
     - 选择 **"Commit directly to the main branch"**
   - 点击 **"Commit changes"**

   **第二批**：手动创建 workflow 文件
   - 在仓库页面点击 **"Add file"** → **"Create new file"**
   - 文件名输入：`.github/workflows/build.yml`
     （注意：前后都有斜杠，GitHub 会自动创建子目录）
   - 把 `AIDbg-source/.github/workflows/build.yml` 的**完整内容**复制粘贴到编辑器里
   - 滚动到底部 "Commit new file"，点 **"Commit new file"**

   **第三批**：手动创建 .gitignore
   - 再次点击 **"Add file"** → **"Create new file"**
   - 文件名：`.gitignore`
   - 把 `AIDbg-source/.gitignore` 内容粘贴进去
   - 点 **"Commit new file"**

   **第四批**：手动创建 .gitattributes
   - 再次点击 **"Add file"** → **"Create new file"**
   - 文件名：`.gitattributes`
   - 把 `AIDbg-source/.gitattributes` 内容粘贴进去
   - 点 **"Commit new file"**

> ⚠️ **务必确认 `.github/workflows/build.yml` 文件存在**，否则 Actions 不会触发。检查方法：仓库页面应该出现一个黄色圆点（表示 Actions 正在跑）。

---

#### 方式 B：用 Git 命令行上传（推荐，一次上传所有文件）

1. 安装 Git：https://git-scm.com/downloads

2. 打开命令行（Windows 用 PowerShell 或 CMD），执行：

```powershell
# 进入解压后的目录
cd D:\path\to\AIDbg-source

# 初始化 Git 仓库
git init
git branch -M main

# 添加所有文件（包括 .github 这种隐藏目录）
git add .
git commit -m "Initial commit: AIDbg plugin source"

# 关联你的 GitHub 仓库（替换成你的实际地址）
git remote add origin https://github.com/your-username/AIDbg.git

# 推送
git push -u origin main
```

第一次推送时会弹出 GitHub 登录窗口，输入账号或用浏览器授权即可。

> 💡 如果提示 `Authentication failed`，参考第 4 节"常见问题"。

---

## 步骤 3：触发 GitHub Actions 编译

### 3.1 自动触发（推送时自动跑）

只要你完成了步骤 2 并 push 到 main 分支，GitHub Actions 会**自动开始编译**。

### 3.2 查看构建进度

1. 进入你的仓库页面：`https://github.com/your-username/AIDbg`

2. 点击顶部标签 **"Actions"**

3. 左侧 workflow 列表选 **"Build AIDbg"**

4. 你会看到一次构建记录（绿色对勾 = 成功，黄色圆点 = 进行中，红叉 = 失败）

5. 点击进入构建详情，左侧能看到两个并行的 job：
   - **build (x64)** → 产出 `AIDbg.dp64`
   - **build (x86)** → 产出 `AIDbg.dp32`

6. 点开任一 job 可以看到实时日志

7. 整个过程通常 **4-6 分钟**

### 3.3 手动触发（如果需要重新编译）

如果你想不修改代码就重新编译：

1. 进入 **Actions** → **Build AIDbg**
2. 右侧点击 **"Run workflow"** 按钮
3. 选择 `main` 分支，点 **"Run workflow"**

---

## 步骤 4：下载编译产物

GitHub 提供两种下载方式：

### 方式 A：下载 Artifact（每次构建都会产生）

**适合**：日常开发，想拿最新版

1. 进入 Actions → 点开某次构建
2. 拉到页面最底部，有个 **"Artifacts"** 区域
3. 看到两个可下载项：
   - **AIDbg-x64** → 点击下载，得到 `AIDbg-x64.zip`，解压后是 `AIDbg.dp64`
   - **AIDbg-x86** → 点击下载，得到 `AIDbg-x86.zip`，解压后是 `AIDbg.dp32`

> ⚠️ Artifact 默认保留 90 天后自动删除。

### 方式 B：下载 Release（推荐，长期保存）

**适合**：稳定版发布，长期可访问

需要打 tag 触发：

1. 在本地仓库执行（或用 GitHub 网页操作）：
   ```powershell
   git tag v0.1.0
   git push origin v0.1.0
   ```

   或者在 GitHub 网页：
   - 仓库 → **Releases** → **Draft a new release**
   - **Choose a tag** → 输入 `v0.1.0` → **Create new tag**
   - **Publish release**

2. GitHub Actions 检测到 tag 推送，会**额外**触发 release 构建：
   - 编译 x64 和 x86
   - 自动创建 GitHub Release 页面
   - 上传 `AIDbg-x64.zip` 和 `AIDbg-x86.zip` 作为 release assets

3. 进入 `https://github.com/your-username/AIDbg/releases` 即可看到发布版

4. 永久下载链接形如：
   ```
   https://github.com/your-username/AIDbg/releases/download/v0.1.0/AIDbg-x64.zip
   ```

---

## 步骤 5：部署到 x64dbg

1. 下载并解压 `AIDbg-x64.zip`（如果你用 64 位 x64dbg）

2. 把 `AIDbg.dp64` 复制到 x64dbg 的插件目录：
   ```
   <你的x64dbg根目录>\x64\plugins\AIDbg.dp64
   ```

3. 启动 x64dbg，日志窗口应该出现：
   ```
   [AIDbg] AIDbg v1 loaded (pluginHandle: X)
   [AIDbg] LLM providers initialized. Active: glm
   [AIDbg] Commands registered: ai ask / ai explain / ai vuln / ai config / ai clear
   ```

4. 在命令栏输入 `ai config`，配置 API Key

5. 反汇编窗口右键 → "AI: Explain this instruction" 测试

---

## 常见问题

### Q1: 推送时提示 `Authentication failed`

GitHub 从 2021 年起不再支持密码认证，需要用 **Personal Access Token (PAT)**：

1. 访问 https://github.com/settings/tokens
2. 点 **"Generate new token"** → 选 **"Generate new token (classic)"**
3. Note: `AIDbg push`
4. Expiration: 90 days
5. 勾选 **`repo`** 权限（含全部子项）
6. 拉到底点 **"Generate token"**
7. **复制 token**（页面关闭后无法再看到）
8. 推送时用 token 代替密码：
   ```
   git push -u origin main
   Username: your-username
   Password: <粘贴刚才的 token>
   ```

或用 Git Credential Manager（Git for Windows 自带）会自动弹浏览器登录。

### Q2: Actions 构建失败

进入 Actions → 点开失败的构建 → 查看红色步骤的日志，常见错误：

| 错误信息 | 原因 | 解决方案 |
|---------|------|---------|
| `CMake Error: Could not find SDK` | 下载 SDK 失败（网络问题） | 重新触发 workflow（Actions → Run workflow） |
| `fatal error C1083: Cannot open include file` | 源码上传不完整 | 检查仓库文件列表，确保所有 `.cpp/.h` 都上传了 |
| `LNK2038: RuntimeLibrary mismatch` | 静态运行时不匹配 | 不应出现，CMake 已配置 /MT，重新检查 |
| `MSBuild version not found` | runner 环境问题 | 重新触发 |

### Q3: Actions 没有自动触发

检查：
1. `.github/workflows/build.yml` 文件**必须**在 `main` 分支上
2. 文件路径必须**精确**是 `.github/workflows/build.yml`（注意有点号）
3. 文件内容必须以 `name: ` 开头
4. YAML 缩进必须正确（用空格，不能用 Tab）

验证：仓库根目录下应能看到 `.github` 文件夹，点进去有 `workflows` 文件夹，再点进去有 `build.yml` 文件。

### Q4: Actions 用量限制

- **公开仓库**：免费，无限分钟数 ✅
- **私有仓库**：免费 2000 分钟/月（GitHub Free 账号）
- 一次构建大约消耗 10 分钟（x64 + x86 各 5 分钟）

### Q5: 想修改代码后重新编译

```powershell
# 修改本地代码后
git add .
git commit -m "Update xxx"
git push
```

推送后 Actions 自动重新跑，5 分钟后下载新的 artifact 即可。

### Q6: 下载的 zip 解压报错"文件损坏"

可能是网络问题导致下载不完整，重新下载即可。也可以用 release 版本（更稳定）。

---

## 流程总结（一张图）

```
解压 AIDbg-source.zip
        │
        ├─ 方式 A：网页上传到 GitHub
        │   ├─ 上传所有源文件
        │   ├─ 手动创建 .github/workflows/build.yml
        │   ├─ 手动创建 .gitignore
        │   └─ 手动创建 .gitattributes
        │
        └─ 方式 B：git push（推荐）
            ├─ git init
            ├─ git add .
            ├─ git commit
            └─ git push
                    │
                    ▼
        GitHub Actions 自动触发
                    │
                    ├─ build (x64) → AIDbg.dp64
                    └─ build (x86) → AIDbg.dp32
                    │
                    ▼
        下载 Artifacts（每次构建都有）
                    │
                    ▼
        或打 tag → 自动创建 Release（长期保存）
                    │
                    ▼
        拷贝 .dp64 / .dp32 到 x64dbg/plugins/
                    │
                    ▼
        启动 x64dbg → ai config → 测试
```

---

## 我已经准备好这些文件

`AIDbg-source.zip` 内已包含：

- ✅ `.github/workflows/build.yml` — GitHub Actions 配置（已优化）
- ✅ `.gitignore` — 忽略编译产物
- ✅ `.gitattributes` — Git 属性
- ✅ `README.md` / `BUILD_VS2022.md` — 文档
- ✅ 全部源码（36 个文件）

**你现在要做的**：把整个 `AIDbg-source/` 目录的内容上传到你新建的 GitHub 仓库即可。

> 💡 **最关键的一点**：务必确保 `.github/workflows/build.yml` 这个文件上传成功，因为它是隐藏目录，网页上传可能会跳过！最稳妥的方式是用 `git push`。
