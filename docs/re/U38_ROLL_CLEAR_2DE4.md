# Unit 0x38 entry 108: roll and clear at +0x2DE4

## Original extent and ownership

The bounded preimplementation `deja "poolrad U38 +2DE4 entry108 helper +0000 roll +0048 strict C parity"` query timed out without a prior-session match. This work reuses the independently documented [U9E entry-8 roll ABI](U9E_ENTRY8_ROLL.md), [U38 private +0000 helper](OVERLAY_UNIT38_HELPER_0000.md), and [U38 effect table installer](U38_EFFECTS_INSTALL_31A2.md); their relevant bytes and frames were checked again here.

Ignored local DOS v1.3 inputs are `GAME.OVR` (206,105 bytes, SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`) and `START.EXE` (65,231 bytes, SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`). The complete public entry is U38 `[+0x2DE4,+0x2E08)`, `GAME.OVR` file `[0x11A3F,0x11A63)`, 36 bytes / 19 contiguous instructions, SHA-256 `7c0e900e0e89d330ef9f35ad0dcfa3550859a8eb46f4e1e311b40b585f591943`. Its only loader fixup is the segment word at +0x2DF0 for the far call at +0x2DED to loaded `:0048`; the stored original segment is not used as a run-time assumption. The two near calls at +0x2DFA and +0x2E01 reach the private +0x0000 helper, whose 31 original bytes have SHA-256 `f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240`.

The installed U38 stub is `0x41:0x023C`, written at `DS:6A18` as effect table item `124`. A fixup scan found no direct far call to this stub. This supports an indirect table route, but no natural game caller frame was reached.

## Decoded behavior

The entry pushes words whose low bytes are 1 and `0x64`, retaining incoming `AH`, and far-calls the loaded U9E roll stub. U9E entry 8 consumes those two words with `RETF 4`. The entry compares returned `AL` unsigned with `0x1E`: above that threshold it returns; otherwise it calls private +0x0000 with keys `0x0B` and `0x35` in order. Each near call uses `PUSH CS; CALL`, so the helper's `RETF 2` consumes the key word. The helper clears `DS:6818` and `DS:6817` when its key equals the current `DS:6817`; it also writes its loaded latch to `AL` and leaves compare flags. The entry returns with `RETF 10`.

`pr_u38_roll_clear_2de4` models the complete wrapper and both helper executions with a 1 MiB RAM image and 14-field CPU state. Its one callback receives the exact far-call entry frame at the independently loaded segment's `:0048`; successful callbacks execute the callee return and must land at caller `CS:2DF2`. Refusal leaves that pending far frame intact. The C helper path retains the near-call stack writes, `AL` and arithmetic flags.

## Measured parity and limits

The original-byte Unicorn checker patches only the loaded segment word and runs both the pinned wrapper and the original private helper. Across 192 cases it compares the complete 1 MiB RAM, all 14 CPU fields, the far callback entry frame, and both near helper entry frames. Results cover 0, 1, `0x0B`, `0x1E`, `0x1F`, `0x7F`, `0x80`, and `0xFF`; latches cover 0, `0x0B`, `0x35`, `0x58`, `0xFF`, and `0x1E`; four DS/SS/stack/loaded-segment layouts are exercised. A deterministic `MOV AL,imm8; RETF 4` roll stub supplies the result for the comparison.

Four DOSBox-X normal-core synthetic far-entry frames execute the same pinned wrapper and helper with a separate loaded roll segment. They match original-byte Unicorn over all 14 CPU fields, 52 stack bytes, and `DS:6817/6818`. The ignored capture lives in `resources/u38-2de4-parity/native/`, SHA-256 `ef93aff001f9cc235d785a6663c2655062bf110bb4a9e4be51a1f672e821b838`. The focused CTest passes C11 `-Wall -Wextra -Wpedantic -Werror` with ASan/UBSan.

The native frames are synthetic. Real U9E random-seed effects are not composed here, and no naturally reached item-124 game frame has been captured. Game identity and strict-function count remain provisional pending integration and PM review.
