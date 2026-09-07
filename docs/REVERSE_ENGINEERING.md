# Darkest Dungeon 1 — Stall/Reinforcement HUD Reverse Engineering Notes

Target:
- Game: Darkest Dungeon® (Steam appid `262060`)
- Build tested: Steam buildid `24960041`
- Executable: `_windows/win64/Darkest.exe` (PE32+, x64)
- Module base example: `0x00007FF73D3C0000`

## Offsets (relative to `Darkest.exe` module base)

| Meaning | RVA / expression |
| --- | --- |
| Global app-state pointer | `module + 0x1179A08` (read 8-byte pointer) |
| Stall counter (int32) | `*(appState + 0x4B7C)` |
| Stall accelerated flag | `appState + 0x4B80` (byte/int) |
| Stress threshold | `module + 0x2AC78B0` (default `3`) |
| Summon/reinforcement threshold | `module + 0x2AC78B4` (default `4`) |
| Reset threshold | `module + 0x2AC78B8` (default `4`) |
| Extra-round threshold | `module + 0x2AC78BC` |
| Extra-round amount | `module + 0x2AC78C0` |
| Accelerated-only consecutive increment config | `module + 0x2AC7AC4` |
| Accelerated-only increment config | `module + 0x2AC7AC8` |

## Related code functions (RVA)

- `0x5A2880` — stall-active/condition check
- `0x5A67F0` — stall processing (applies stress, reinforcements, resets)
- Callers around `0x757DED`, `0x757E15`, `0x757E2B`, `0x757E46`

## How to read the counter

```
app = *(u64*)(base + 0x1179A08)
count = *(i32*)(app + 0x4B7C)
remaining = summon_threshold - count   // default: 4 - count
```

## Observed semantics

- The stall counter is global for the current battle, not per monster.
- Some monsters can set `.disable_stall_penalty True` (bosses/no reinforcement).
- Some monsters can set `.accelerate_stall_penalty True` (accelerated stall).
- In disassembly:
  - The caller increments `stall_count` when the current round is flagged as a stall round.
  - `ProcessStall` (`0x5A67F0`):
    - `count >= summon_threshold (4)` → executes reinforcement/summon path.
    - `count < summon_threshold` and `count >= stress_threshold (3)` → applies stall stress.
    - `count >= reset_threshold (4)` → resets count to `0` and clears the stall flag.
- Therefore visible “distance to reinforcement” can be:
  - `summon_threshold - count`, clamped at `0`.
- If the accelerated flag is set and count is positive, the next stall round may immediately trigger reinforcement, so the HUD displays `1` instead of the raw remaining formula.

## Dynamic verification example

Environment:
- First-room test battle (`-firstroombattle skeleton_defender_A`)
- 4 heroes vs 1 Skeleton Defender
- `ProbeStall.exe <pid>` reads values

Observation after completing rounds with only one attack and not finishing the enemy:

| Stage | stall count | remaining |
| --- | --- | --- |
| At first stall-active start | 1 | 3 |
| After one more stall round | 2 | 2 |

This confirms `+0x4B7C` increments as the real stall counter.

## Note on game updates

These offsets were found for a specific Darkest Dungeon build. After a game update, verify the module version and repeat the memory scan before assuming the HUD still works.
