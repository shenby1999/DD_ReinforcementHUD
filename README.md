# Darkest Dungeon Reinforcement HUD

暗黑地牢 增援倒计时 HUD

A small launcher + native overlay HUD for **Darkest Dungeon (Steam appid 262060)** that reads the game's real stall/reinforcement counter and shows how many rounds remain before reinforcements are summoned.

这是一个通过读取游戏真实 stall/增援计数，在屏幕顶部显示“距离下一波增援还有几回合”的小型启动器 + OpenGL 覆盖层 HUD。

> ⚠️ This project uses DLL injection and memory address reading. It is intended for personal study/testing. Antivirus software may report a false positive. Use at your own risk.
>
> ⚠️ 本项目使用 DLL 注入和内存地址读取，仅供个人学习/测试使用。部分杀毒软件可能误报，请自行承担使用风险。

---

## Features / 功能

- Reads the real stall/reinforcement counter from the running Darkest Dungeon process.
- Shows the remaining rounds until the next reinforcement wave at the top-center of the game screen.
- Display rules:
  - `3` → amber / gold
  - `2` → orange
  - `1` → bright red
  - No reinforcement / not started → `--`
- If an accelerated stall flag is active, a count of `1` is shown instead of a misleading larger number.
- Auto-detects Steam installation paths and writes `game_path.txt`.
- If the game is already running, the launcher attaches to it.
- Duplicate-injection detection prevents the HUD from being injected twice.
- Fixed-size HUD scales relative to the OpenGL viewport height.

---

## Usage / 使用方法

### Ready-to-run / 直接运行

1. Install and start Steam. Make sure Darkest Dungeon is installed.
2. Double-click `启动HUD.bat`.
3. If the launcher cannot find the game, create `game_path.txt` next to `StartWithHud.exe` with the full path to `Darkest.exe`, for example:
   ```
   D:/Steam/steamapps/common/DarkestDungeon/_windows/win64/Darkest.exe
   ```
4. If the game is already running, the launcher will attach to it and inject the HUD. If the HUD is already loaded, it will skip injection.

### Files / 文件说明

| File | Description |
| --- | --- |
| `StartWithHud.exe` | Launcher: starts/attaches the game and auto-injects the HUD |
| `Injector.exe` | Native DLL injector |
| `ReinforcementHudColor.dll` | OpenGL overlay HUD plugin |
| `启动HUD.bat` | Batch launcher |
| `game_path.txt` | Optional: full path to `Darkest.exe` |

---

## How it works / 实现原理

- Darkest Dungeon stores a global stall counter in memory.
- The HUD DLL hooks `SDL_GL_SwapWindow` from `SDL2.dll` through the game's IAT.
- On every rendered frame, the HUD reads the stall counter and draws a seven-segment digit using modern OpenGL.
- The displayed number is normally `summon_threshold - stall_count`. Default threshold is `4`, so the first stall round shows `3`.
- When the accelerated stall flag is set and the count is positive, the HUD shows `1` because the next stall round can trigger reinforcement immediately.
- When the counter is not active, the HUD draws `--`.

Reverse-engineering notes are in [docs/REVERSE_ENGINEERING.md](docs/REVERSE_ENGINEERING.md).

---

## Source code / 源代码

Source files are under `src/`:

- `src/StartWithHud.cs` — C# launcher
- `src/injector.cpp` — native x64 DLL injector
- `src/GlHudNice.cpp` — OpenGL overlay HUD DLL

### Build prerequisites / 编译环境

- Windows 10/11 (64-bit)
- C# compiler (e.g. .NET Framework `csc.exe`, usually at `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe`)
- MinGW-w64 or LLVM MinGW with OpenGL headers (`GL/gl.h`, `GL/glcorearb.h`)

### Build commands / 编译命令

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

- This tool depends on memory offsets of a specific game build. If Darkest Dungeon updates, the offsets may change and need to be adjusted.
- The HUD is not a Steam Workshop mod. It is a separate external launcher/injector.
- For anti-cheat/ToS concerns: Darkest Dungeon is an offline/single-player game; this tool does not modify game files or gameplay. Still, use at your own risk.
- Some antivirus programs may flag DLL injectors. You can verify the source and build the binaries yourself.

---

## Disclaimers / 免责声明

- “Darkest Dungeon” is a trademark of Red Hook Studios. This project is an unofficial fan tool.
- This tool is provided “as is”, without warranty of any kind.

---

## Repository / 仓库

- GitHub: <https://github.com/shenby1999/DD_ReinforcementHUD>
