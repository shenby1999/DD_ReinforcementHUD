# Darkest Dungeon Reinforcement HUD

# 暗黑地牢 增援倒计时 HUD

## Introduction / 简介

**English:** A small launcher + native overlay HUD for **Darkest Dungeon (Steam appid 262060)**. It reads the game's real stall/reinforcement counter and shows how many rounds remain before reinforcements are summoned at the top-center of the game screen.

**中文：** 这是一个用于 **暗黑地牢（Steam appid 262060）** 的小型启动器 + 原生 OpenGL 覆盖层 HUD。它会读取游戏真实的 stall/增援计数，并在游戏画面顶部中间显示“距离下一波增援还有几回合”。

> ⚠️ **English:** This project uses DLL injection and memory address reading. It is intended for personal study/testing. Antivirus software may report a false positive. Use at your own risk.
>
> ⚠️ **中文：** 本项目使用 DLL 注入和内存地址读取，仅供个人学习/测试使用。部分杀毒软件可能误报，请自行承担使用风险。

---

## Features / 功能特点

### 中文

- 从正在运行的暗黑地牢进程中读取真实的 stall/增援计数。
- 在游戏画面顶部中间显示距离下一波增援还有多少回合。
- 显示规则：
  - `3` → 金色/琥珀色
  - `2` → 橘色/橙色
  - `1` → 亮红色
  - 无增援 / 未开始 → `--`
- 如果检测到加速 stall（accelerated stall）标志，则显示 `1`，避免显示误导性的较大数字。
- 自动检测 Steam 安装路径，并写入 `game_path.txt`。
- 如果游戏已经在运行，启动器会直接附加到已运行的进程。
- 有重复注入检测，避免 HUD 被重复注入。
- HUD 尺寸会根据 OpenGL 视口高度等比缩放，在不同分辨率下保持固定显示比例。

### English

- Reads the real stall/reinforcement counter from the running Darkest Dungeon process.
- Shows the remaining rounds until the next reinforcement wave at the top-center of the game screen.
- Display rules:
  - `3` → amber / gold
  - `2` → orange
  - `1` → bright red
  - No reinforcement / not started → `--`
- If an accelerated stall flag is active, `1` is shown instead of a misleading larger number.
- Auto-detects Steam installation paths and writes `game_path.txt`.
- If the game is already running, the launcher attaches to it.
- Duplicate-injection detection prevents the HUD from being injected twice.
- The HUD scales relative to the OpenGL viewport height so it looks consistent across resolutions.

---

## Usage / 使用方法

### 中文

#### 直接运行

1. 确保已安装并启动 Steam，同时已安装暗黑地牢。
2. 双击 `启动HUD.bat`。
3. 如果启动器找不到游戏，请在 `StartWithHud.exe` 同目录下创建 `game_path.txt`，写入 `Darkest.exe` 的完整路径，例如：
   ```
   D:/Steam/steamapps/common/DarkestDungeon/_windows/win64/Darkest.exe
   ```
4. 如果游戏已经运行，启动器会附加到游戏并自动注入 HUD；如果 HUD 已注入，则会自动跳过，避免重复注入。

#### 文件说明

| 文件 | 说明 |
| --- | --- |
| `StartWithHud.exe` | 启动器：启动/附加游戏并自动注入 HUD |
| `Injector.exe` | 原生 DLL 注入器 |
| `ReinforcementHudColor.dll` | OpenGL 覆盖层 HUD 插件 |
| `启动HUD.bat` | 一键启动脚本 |
| `game_path.txt` | 可选：指定 `Darkest.exe` 的完整路径 |

### English

#### Ready-to-run

1. Install and start Steam. Make sure Darkest Dungeon is installed.
2. Double-click `启动HUD.bat`.
3. If the launcher cannot find the game, create `game_path.txt` next to `StartWithHud.exe` with the full path to `Darkest.exe`, for example:
   ```
   D:/Steam/steamapps/common/DarkestDungeon/_windows/win64/Darkest.exe
   ```
4. If the game is already running, the launcher attaches to it and injects the HUD. If the HUD is already loaded, it skips injection.

#### Files

| File | Description |
| --- | --- |
| `StartWithHud.exe` | Launcher: starts/attaches the game and auto-injects the HUD |
| `Injector.exe` | Native DLL injector |
| `ReinforcementHudColor.dll` | OpenGL overlay HUD plugin |
| `启动HUD.bat` | Batch launcher |
| `game_path.txt` | Optional: full path to `Darkest.exe` |

---

## How it works / 实现原理

### 中文

- 暗黑地牢会把 stall 计数保存在游戏进程的内存中。
- HUD DLL 通过游戏的 IAT 钩住 `SDL2.dll` 导出的 `SDL_GL_SwapWindow`。
- 每一帧渲染时，HUD 读取 stall 计数，并用现代 OpenGL 绘制七段数码管数字。
- 正常情况下显示 `增援阈值 - stall 计数`。默认阈值是 `4`，所以第一次 stall 回合显示 `3`。
- 当加速 stall 标志生效且计数大于 `0` 时，HUD 显示 `1`，因为下一轮 stall 就可能直接触发增援。
- 当计数未激活/没有增援时，HUD 显示 `--`。

逆向工程笔记见 [docs/REVERSE_ENGINEERING.md](docs/REVERSE_ENGINEERING.md)。

### English

- Darkest Dungeon stores a global stall counter in memory.
- The HUD DLL hooks `SDL_GL_SwapWindow` from `SDL2.dll` through the game's IAT.
- On every rendered frame, the HUD reads the stall counter and draws a seven-segment digit using modern OpenGL.
- The displayed number is normally `summon_threshold - stall_count`. Default threshold is `4`, so the first stall round shows `3`.
- When the accelerated stall flag is set and the count is positive, the HUD shows `1` because the next stall round can trigger reinforcement immediately.
- When the counter is not active, the HUD draws `--`.

Reverse-engineering notes are in [docs/REVERSE_ENGINEERING.md](docs/REVERSE_ENGINEERING.md).

---

## Source code / 源代码

### 中文

源码位于 `src/` 目录：

- `src/StartWithHud.cs` — C# 启动器源码
- `src/injector.cpp` — 原生 x64 DLL 注入器源码
- `src/GlHudNice.cpp` — OpenGL 覆盖层 HUD DLL 源码

#### 编译环境

- Windows 10/11（64 位）
- C# 编译器（例如 .NET Framework 的 `csc.exe`，通常位于 `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe`）
- MinGW-w64 或 LLVM MinGW，并带有 OpenGL 头文件（`GL/gl.h`、`GL/glcorearb.h`）

#### 编译命令

```bat
:: StartWithHud.exe（启动器）
"C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /optimize+ /out:StartWithHud.exe src\StartWithHud.cs

:: Injector.exe（注入器，静态链接以避免 MSVC 运行库依赖）
x86_64-w64-mingw32-g++ -O2 -static -static-libgcc -static-libstdc++ src\injector.cpp -o Injector.exe

:: ReinforcementHudColor.dll（HUD 插件）
x86_64-w64-mingw32-g++ -shared -O2 -static -static-libgcc -static-libstdc++ src\GlHudNice.cpp -o ReinforcementHudColor.dll -lopengl32
```

如果你使用 LLVM MinGW，可以把 `x86_64-w64-mingw32-g++` 替换为 `clang++ --target=x86_64-w64-mingw32`。

编译完成后，请把 `StartWithHud.exe`、`Injector.exe`、`ReinforcementHudColor.dll` 放在与 `启动HUD.bat` 相同的目录中。

### English

Source files are under `src/`:

- `src/StartWithHud.cs` — C# launcher
- `src/injector.cpp` — native x64 DLL injector
- `src/GlHudNice.cpp` — OpenGL overlay HUD DLL

#### Build prerequisites

- Windows 10/11 (64-bit)
- C# compiler (e.g. .NET Framework `csc.exe`, usually at `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe`)
- MinGW-w64 or LLVM MinGW with OpenGL headers (`GL/gl.h`, `GL/glcorearb.h`)

#### Build commands

```bat
:: StartWithHud.exe
"C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /optimize+ /out:StartWithHud.exe src\StartWithHud.cs

:: Injector.exe (static to avoid MSVC runtime dependencies)
x86_64-w64-mingw32-g++ -O2 -static -static-libgcc -static-libstdc++ src\injector.cpp -o Injector.exe

:: ReinforcementHudColor.dll
x86_64-w64-mingw32-g++ -shared -O2 -static -static-libgcc -static-libstdc++ src\GlHudNice.cpp -o ReinforcementHudColor.dll -lopengl32
```

If you use LLVM MinGW, replace `x86_64-w64-mingw32-g++` with `clang++ --target=x86_64-w64-mingw32`.

After building, put `StartWithHud.exe`, `Injector.exe`, and `ReinforcementHudColor.dll` in the same folder as `启动HUD.bat`.

---

## Important notes / 注意事项

### 中文

- 本工具依赖特定游戏版本的内存偏移。如果暗黑地牢更新，偏移量可能变化，需要重新适配。
- HUD 不是 Steam 创意工坊 Mod，而是一个独立的外部启动器/注入器。
- 暗黑地牢是单机游戏，本工具不修改游戏文件，也不会改变游戏玩法。请自行确认是否可接受后使用。
- 部分杀毒软件可能把 DLL 注入器标记为风险程序。你可以自行查看源码并重新编译。

### English

- This tool depends on memory offsets of a specific game build. If Darkest Dungeon updates, the offsets may change and need to be adjusted.
- The HUD is not a Steam Workshop mod. It is a separate external launcher/injector.
- Darkest Dungeon is an offline/single-player game; this tool does not modify game files or gameplay. Still, use at your own risk.
- Some antivirus programs may flag DLL injectors. You can verify the source and build the binaries yourself.

---

## Disclaimers / 免责声明

### 中文

- “Darkest Dungeon”（暗黑地牢）是 Red Hook Studios 的商标。本项目是非官方粉丝工具。
- 本工具按“现状”提供，不附带任何形式的担保。

### English

- “Darkest Dungeon” is a trademark of Red Hook Studios. This project is an unofficial fan tool.
- This tool is provided “as is”, without warranty of any kind.

---

## Repository / 仓库

- GitHub: <https://github.com/shenby1999/DD_ReinforcementHUD>
