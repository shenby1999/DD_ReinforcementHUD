# Darkest Dungeon HUD 偏移更新指南

当暗黑地牢更新后，如果出现以下现象：

- `StartHUD.bat` 启动后游戏注入即闪退
- 先启动游戏再注入时游戏异常
- HUD 能注入但数字不变化

通常是游戏内存偏移变化，HUD 需要重新适配。

## 管线文件

| 文件 | 用途 |
| --- | --- |
| `tools/offsets.json` | 保存已知游戏版本的偏移 |
| `tools/Update-HUD.ps1` | 一条命令完成改偏移、编译、更新工坊内容、打包发布 |
| `docs/OFFSET_UPDATE_GUIDE.md` | 本指南 |

## 第一步：确认游戏版本

读取 Steam manifest：

```text
<Steam>\steamapps\appmanifest_262060.acf
```

找到：

```text
"buildid" "25309191"
```

例如更新后 buildid 可能变成新的数字。

## 第二步：检查旧偏移是否失效

快速检查方法：启动游戏，用内存读取工具读取旧偏移。

旧版偏移：

```text
appState 指针      module + 0x117DB48
stall 计数         appState + 0x2E4
stall 加速标志     appState + 0x2E8
增援阈值           module + 0x2ACBC04
压力阈值           module + 0x2ACBC00
重置阈值           module + 0x2ACBC08
```

如果读取到的值明显异常，例如：

- appState 指针不是合法堆地址
- 计数非常大或负数
- 阈值不是 3/4/4

说明偏移已变化。

## 第三步：重新定位阈值

在 `Darkest.exe` 中搜索规则键字符串：

```text
stall_round_stress_effect_threshold
stall_round_summon_effect_threshold
stall_round_reset_threshold
stall_extra_round_size_threshold
stall_extra_round_size_amount
```

推荐工具：

- 任意支持字符串搜索的十六进制编辑器
- `grep -aob`
- `objdump` / IDA / Ghidra

找到字符串后，在反汇编中搜索该字符串的引用。初始化代码通常会把默认值写入连续的全局变量，例如：

```asm
movl $0x3, <stress_threshold>
movl $0x4, <summon_threshold>
movl $0x4, <reset_threshold>
movl $0x2, <extra_round_threshold>
movl $0x1, <extra_round_amount>
```

记录下来，例如当前版本的：

```text
stress_threshold   = module + 0x2ACBC00
summon_threshold   = module + 0x2ACBC04
reset_threshold    = module + 0x2ACBC08
```

## 第四步：定位 appState 全局指针

目标：找到保存 appState 对象的全局指针，以及 appState 内部的计数和标志偏移。

当前版本已经确认：

```text
appState 全局指针  = module + 0x117DB48
stall 计数         = appState + 0x2E4
stall 加速标志     = appState + 0x2E8
```

重新定位的方法：

1. 用 `ProcessStall` 相关逻辑入口反查。
   - 先找到引用 `summon_threshold` 全局地址的代码。
   - 该代码会读取 stall 计数并和阈值比较。
2. 从 `ProcessStall` 的调用者向上回溯，找到保存 appState 的对象。
3. 用调试器（例如 `lldb`）在相关函数入口断下，读取 `rcx` / `rsi` 中的对象地址。
4. 在模块 `.data` 中搜索哪个全局变量保存了这个对象地址。
5. 在对象内部确认：
   - 计数字段初始为 `0`
   - 加速标志初始为 `0`
   - 处理函数会读取计数、比较阈值，并在重置时把计数和标志清零。

当前版本反查结果：

```text
全局指针 RVA       : 0x117DB48
计数偏移           : 0x2E4
加速标志偏移       : 0x2E8
```

## 第五步：写入配置文件

编辑：

```text
tools/offsets.json
```

为新的 buildid 增加一项，例如：

```json
"25309191": {
  "appStatePtrRVA": "0x117DB48",
  "stallCountOffset": "0x2E4",
  "stallFlagOffset": "0x2E8",
  "summonThresholdRVA": "0x2ACBC04",
  "stressThresholdRVA": "0x2ACBC00",
  "resetThresholdRVA": "0x2ACBC08"
}
```

## 第六步：一键更新

在仓库根目录运行：

```powershell
.\tools\Update-HUD.ps1 -BuildId 25309191
```

脚本会：

1. 按 `tools/offsets.json` 修改 `src/GlHudNice.cpp`
2. 重新编译 `ReinforcementHudColor.dll`
3. 更新 Steam 创意工坊内容目录
4. 重新生成发布 zip

如果需要同时提交 GitHub：

```powershell
.\tools\Update-HUD.ps1 -BuildId 25309191 -PushGit
```

如果需要同时推送 Steam 创意工坊：

```powershell
.\tools\Update-HUD.ps1 -BuildId 25309191 -PublishWorkshop
```

一条命令全部完成：

```powershell
.\tools\Update-HUD.ps1 -BuildId 25309191 -PushGit -PublishWorkshop
```

## 第七步：测试

1. 启动游戏。
2. 注入 HUD。
3. 进入一场会增援的战斗。
4. 确认：
   - 数字从 3 开始递减
   - `2` 显示橙色
   - `1` 显示红色
   - 无增援时显示 `--`
   - 游戏不闪退

## 快速回滚

如果新偏移导致异常，可以在 `tools/offsets.json` 中把旧版本条目保留，用旧 buildid 重新编译对应版本，或者直接 git 回退：

```powershell
git log --oneline
git revert <commit>
```
