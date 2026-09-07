# Darkest Dungeon Reinforcement HUD / 暗黑地牢 增援倒计时 HUD

在游戏画面顶部显示“距离下一波增援还有几回合”的 HUD。  
A small HUD that shows how many rounds remain before the next reinforcement wave.

---

## 快速使用 / Quick Start

1. 把整个文件夹里的内容解压到暗黑地牢**游戏根目录**，和 `_windows` 文件夹同级。  
   Unpack all files into the Darkest Dungeon game root, in the same folder as `_windows`.
2. 双击 `启动HUD.bat`。  
   Double-click `启动HUD.bat`.
3. 如果游戏没开，会自动找到并启动游戏；如果游戏已开，会直接附加并注入 HUD。  
   It starts the game automatically, or attaches if Darkest Dungeon is already running.
4. 进入战斗后，屏幕顶部会显示数字。  
   The number appears at the top-center during battle.

找不到游戏时：在 `StartWithHud.exe` 同目录创建 `game_path.txt`，写入 `Darkest.exe` 完整路径。  
If the game is not found, create `game_path.txt` next to `StartWithHud.exe` containing the full path to `Darkest.exe`.

示例：

```text
D:/Steam/steamapps/common/DarkestDungeon/_windows/win64/Darkest.exe
```

---

## 数字含义 / Display

| 显示 | 含义 | 颜色 |
| --- | --- | --- |
| `3` | 还剩 3 回合出增援 | 金色 / Gold |
| `2` | 还剩 2 回合出增援 | 橙色 / Orange |
| `1` | 还剩 1 回合出增援，或加速状态下下一回合可能触发 | 亮红 / Bright red |
| `--` | 未开始，或本场战斗不会增援 | 金色 / Gold |

---

## 常见问题 / FAQ

- **HUD 没出现？**  
  确保四个文件在同一目录：`StartWithHud.exe`、`Injector.exe`、`ReinforcementHudColor.dll`、`启动HUD.bat`。如果已经在游戏里，先退出游戏再运行。

- **提示找不到游戏？**  
  使用上面的 `game_path.txt` 手动指定游戏路径。

- **杀毒软件报毒？**  
  本工具通过 DLL 注入工作，可能被杀毒软件误报。仅供个人学习/测试使用，请自行判断。

- **游戏更新后失效？**  
  本工具依赖当前游戏版本的内存地址，游戏更新后可能需要重新适配。

---

## 文件说明 / Files

```text
StartWithHud.exe          启动/附加游戏并自动注入 HUD
Injector.exe              原生 DLL 注入器
ReinforcementHudColor.dll OpenGL HUD 插件
启动HUD.bat               一键启动入口
game_path.txt             可选，手动指定 Darkest.exe 路径
```

---

## 开发者 / For developers

- 源码：`src/`  
  Source code: `src/`
- 逆向笔记：`docs/REVERSE_ENGINEERING.md`  
  Reverse-engineering notes: `docs/REVERSE_ENGINEERING.md`

## 免责声明 / Disclaimer

非官方粉丝工具，仅供学习/测试使用，按“现状”提供。  
Unofficial fan tool for personal study/testing, provided as-is.

本程序由 DeepSeek Harness 协助开发。  
This program was developed with assistance from DeepSeek Harness.
