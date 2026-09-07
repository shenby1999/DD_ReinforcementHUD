暗黑地牢 增援倒计时 HUD
========================

功能
----
- 读取游戏真实 stall/增援计数
- 顶部显示距离下一波增援还有几回合
- 数字 3：金色
- 数字 2：橘色
- 数字 1：红色
- 无增援/未开始：显示 --

使用方法
--------
1. 确保 Steam 和游戏已安装。
2. 双击“启动HUD.bat”。
3. 如果启动器找不到游戏，创建 game_path.txt，
   内容写 Darkest.exe 的完整路径，例如：
   D:/Steam/steamapps/common/DarkestDungeon/_windows/win64/Darkest.exe
4. 如果游戏已运行，会自动附加；如果已注入 HUD，会自动跳过。

文件说明
--------
StartWithHud.exe            启动/附加游戏并自动注入 HUD
Injector.exe                原生 DLL 注入器
ReinforcementHudColor.dll   HUD 插件
启动HUD.bat                 正常启动并注入
game_path.txt               可选：指定 Darkest.exe 完整路径

注意事项
--------
- 本工具通过 DLL 注入方式工作，部分杀毒软件可能误报。
- 仅供个人/学习/测试使用。
- 若游戏更新，内存偏移可能变化，需要重新适配。
- 本工具使用DeepSeek Harness辅助开发。
