# Darkest Dungeon 1 — Stall/Reinforcement HUD Reverse Engineering Notes

Target:
- Game: Darkest Dungeon® (Steam appid `262060`)
- Build tested: Steam buildid `25309191` (2026-09-16 update)
- Executable: `_windows/win64/Darkest.exe` (PE32+, x64)
- Module base example: `0x00007FF6FE050000`

## Offsets (relative to `Darkest.exe` module base)

| Meaning | RVA / expression |
| --- | --- |
| Global app-state pointer | `module + 0x117DB48` (read 8-byte pointer) |
| Stall counter (int32) | `*(appState + 0x4B7C)` |
| Stall accelerated flag | `appState + 0x4B80` (byte) |
| Stress threshold | `module + 0x2ACBC00` (default `3`) |
| Summon/reinforcement threshold | `module + 0x2ACBC04` (default `4`) |
| Reset threshold | `module + 0x2ACBC08` (default `4`) |
| Extra-round threshold | `module + 0x2ACBC0C` |
| Extra-round amount | `module + 0x2ACBC10` |
| Accelerated-only consecutive increment config | `module + 0x2ACBE14` |
| Accelerated-only increment config | `module + 0x2ACBE18` |

## Related code functions (RVA)

- `0x5A8F50` — stall processing (applies stress, reinforcements, resets)
- Caller/round-processing code around `0x75A2AD`, `0x75A6C5`, `0x75A6D4`

## How to read the counter

```
app = *(u64*)(base + 0x117DB48)
count = *(i32*)(app + 0x4B7C)
flag = *(u8*)(app + 0x4B80)
remaining = summon_threshold - count   // default: 4 - count
```

## Observed semantics

- The stall counter is global for the current battle, not per monster.
- Some monsters can set `.disable_stall_penalty True` (bosses/no reinforcement).
- Some monsters can set `.accelerate_stall_penalty True` (accelerated stall).
- In disassembly:
  - The caller increments `stall_count` when the current round is flagged as a stall round.
  - `ProcessStall` (`0x5A8F50`):
    - `count >= summon_threshold (4)` → executes reinforcement/summon path.
    - `count < summon_threshold` and `count >= stress_threshold (3)` → applies stall stress.
    - `count >= reset_threshold (4)` → resets count to `0` and clears the stall flag at `+0x2E8`.
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

This confirms the stall counter increments as real stall rounds accumulate. In build `25309191`, the counter is `appState + 0x2E4`.

## Note on game updates

These offsets were found for a specific Darkest Dungeon build. After a game update, verify the module version and repeat the memory scan before assuming the HUD still works.
