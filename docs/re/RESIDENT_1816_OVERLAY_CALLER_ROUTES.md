# Eight overlay call frames for `RES_00BA:1816`

This separate follow-on to [PR #418](https://github.com/huntergdavis/poolrad-decomp/pull/418)
models the **straight-line argument setup** immediately before each of the
eight relocation-backed `CALL FAR 00BA:1816` sites. The original-byte/C
checker compares the complete 1 MiB RAM image and all fourteen 16-bit CPU
registers at the call boundary and after an identical synthetic callee
return. It does not translate the containing overlay functions or execute
the printer inside `1816`.

The ignored DOS v1.3 inputs remain `START.EXE` (65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`)
and `GAME.OVR` (206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`).
`resident_inventory.collect` independently validates those whole-file hashes,
the overlay fixups, and the eight target addresses before the differential.

## Bounded original spans

Each span begins at the first pointer-word push and ends just after the
five-byte far call. The source has five pushes in this order: node segment,
node offset, column word, row word, flag word. The three scalar words are
formed by `MOV AL` followed by `PUSH AX`; their high byte retains the entry
AH. The callee consumes the low bytes, while this harness preserves the
whole pushed words and the stack writes.

| Unit | Start → call | `GAME.OVR` file span | SHA-256 of bounded span | Node source; column, row, flag low bytes |
| --- | --- | --- | --- | --- |
| `05E` | `+0362 → +0372` | `[0x12B5D,0x12B72)` | `705949170e64159e65308a093da662c7ccdba641c83063040916d35e150f0bdb` | `SS:[BP+10]/[BP+0E]`; `23`, `SS:[BP-1]`, `0` |
| `06C` | `+2D3D → +2D4F` | `[0x1C6BC,0x1C6D3)` | `9b6556299a20b24c01613153586505a8a80576c5d87bdb4e44609b31b8adc265` | `DS:5D94/5D92`; `4`, `SS:[BP-16]`, `0` |
| `077` | `+0078 → +0089` | `[0x210B2,0x210C8)` | `53a090581a2123fbb23568b84dd9cd0c62c70caf1b3c427710bf0500f045a15e` | `DS:4718/4716`; `1`, `1`, `0` |
| `077` | `+0DCF → +0DE0` | `[0x21E09,0x21E1F)` | `900299d7985f4aa6f992043c52cae50d0e4d06debc3c5ebd8f80f2caacd8755a` | `DS:5D94/5D92`; `1`, `21`, `0` |
| `077` | `+11BD → +11CC` | `[0x221F7,0x2220B)` | `8228ea1ac5f4f4b8516524c591b7c0d89fe51b97dd687c7a3ed13d632f3165de` | `SS:[BP-3D]/[BP-3F]`; `1`, `1`, `1` |
| `077` | `+2AA5 → +2AB6` | `[0x23ADF,0x23AF5)` | `85e5363baa285a6b29c6320f0cc19b640a55280dda3ab2cab72787322ddb8ac3` | `DS:5D94/5D92`; `1`, `1`, `1` |
| `07F` | `+1B3F → +1B50` | `[0x267C8,0x267DE)` | `05564fba780ad047a86fc208ca800d9cd59c8bb945b426e8f2aa8850f3f2162e` | `DS:5D94/5D92`; `1`, `1`, `0` |
| `086` | `+33B4 → +33C3` | `[0x2A3CC,0x2A3E0)` | `b18961dbf181e11d63f1d5d26e609f157aad52c30bcd16aebcd8707f1cead347` | `SS:[BP+0E]/[BP+0C]`; `1`, `19`, `0` |

The pointer source lists segment then offset, matching the push order.
`BP-16` in the table means hexadecimal `0x16` (22 decimal), and likewise
`BP-3D` and `BP-3F` are hexadecimal displacements. The static inventory
proves these eight sites exist; it does not prove all eight execute on a
particular game route.

## Differential boundary and synthetic cases

[`resident_1816_caller_routes.c`](../../tools/overlay/resident_1816_caller_routes.c)
models each decoded prefix's memory reads, `MOV AL` changes, five full-word
pushes, and far-call frame. A typed callback observes five push events in
source order, then receives the frame at `CALL_DISPLAY`. That callback is a
synthetic stand-in for the entire resident callee. The model applies the
callee's `RETF 10` stack effect and resumes at the original return address.

[`check_resident_1816_overlay_callers.py`](../../tests/recovered/check_resident_1816_overlay_callers.py)
hash-locks each original span, checks its nine decoded instructions and far
target, and executes it in Unicorn. The original run stops at the far call;
the same synthetic callee changes one scratch byte and AX in both runs.
For each of eight sites, three synthetic entry states vary AH, node offset,
row byte, and stack position, including 16-bit SP wrap. The checker asserts:

1. Exactly the original nine instructions execute to the call.
2. The callback order is node segment, node offset, column, row, flag, call.
3. All five pushed words, the entire 1 MiB RAM image, and all CPU registers
   agree at the call boundary.
4. RAM and CPU agree again after the shared synthetic return. The flags
   comparison masks AF, which these instructions do not define or change.

The measured result is **24/24 full-state bounded-prefix cases passed**.
The code builds its strict-warning C shared library under ignored `resources/`
and keeps the original bytes and RAM in private memory. Reproduce with:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  PYTHONDONTWRITEBYTECODE=1 \
  python3 tests/recovered/check_resident_1816_overlay_callers.py
```

This parity covers the caller prefixes and synthetic callee boundary only.
No strict full-C game-routine or additional game-function credit follows from
it. The complete `0709:0626` printer and the remainder of each overlay
caller still need separate evidence.

Before implementation, bounded `deja "RES_00BA 1816 eight overlay far callers
route harness"` was attempted and timed out without a focused result. The
merged [`1816` callee note](RESIDENT_UNEQUAL_DISPLAY_1816.md) and
[`resident inventory`](../../tools/overlay/resident_inventory.py) supplied
the address leads; each prefix and hash above was checked again against the
ignored original `GAME.OVR`.
