# Unit 0x38 entry 115: conditional remove call at +0x317A

## Original extent and ownership

The bounded preimplementation `deja "Poolrad U38 +317A entry115 remove gate U9E +002A"` lookup timed out without a prior-session result. This work reuses the independently documented [U9E entry-2 ABI](U9E_ENTRY2.md) and [U38 effect-table installer](U38_EFFECTS_INSTALL_31A2.md); the original bytes, loaded call frame, and installer slot were checked independently.

Ignored DOS v1.3 inputs are `GAME.OVR` (206,105 bytes, SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`) and `START.EXE` (65,231 bytes, SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`). The complete U38 public entry is `[+0x317A,+0x319B)`, `GAME.OVR` file `[0x11DD5,0x11DF6)`: 33 bytes / 15 contiguous instructions, SHA-256 `6fea300fcdcd481e8db60540c7d32abbbf2d0102b2c806648b05a086782788ad`. Entry 116 starts at +0x319B, so no following bytes are included. The sole loader fixup in this span is the far-call segment word at +0x3195. The original call at +0x3192 targets U9E entry 2 at loaded `:002A`, whose `RETF 10` consumes five argument words.

The original public stub is `0041:025F`. The U38 installer loads that pointer at +0x3880/+0x3883 and stores its two words at +0x3886/+0x3889 to `DS:6A4C/6A4E`, effect table item 137 relative to base `DS:6828`. The table assignment proves installation, not a naturally reached dispatch frame. A direct far caller and a natural game frame have not been demonstrated. The existing classifier remains `game_or_other_library_unresolved`; no identity or strict-function count is promoted here.

## Decoded behavior and C contract

The entry checks the low byte of its first argument word at `[BP+6]`. Zero skips the call. A nonzero byte pushes the caller's fourth and fifth argument words (`[BP+E]`, `[BP+C]`), changes only `AL` to `0x17` and pushes AX, zeroes DX and AX, then pushes both zero words. It calls the independently loaded U9E entry 2. This creates a far frame with return `CS:3197`, item pointer `0000:0000`, type word `(incoming AH << 8) | 0x17`, and the two forwarded words. After the callee's `RETF 10`, the wrapper pops BP and itself returns with `RETF 10`. It does not inspect the callee's return value.

`pr_u38_remove_gate_317a` translates the complete wrapper using 1 MiB RAM and the 14-field CPU state. A successful callback must execute the loaded callee's far return and land at caller `CS:3197`; callback refusal preserves the pending frame and returns `CALL_ABORT`. The host argument guard returns `ARGUMENT` before mutation. The model preserves the original stack writes, incoming `AH`, comparison/XOR flag effects, and both far-return cleanups. It models the wrapper; the real U9E item-removal body is outside this slice.

## Measured parity and limits

The original-byte Unicorn checker pins both media hashes, verifies inventory entry boundaries and the fixup, then executes the 33 pinned bytes with only that segment word loaded. A deterministic `MOV AX,0xBEEF; RETF 10` at loaded `:002A` stands for the called U9E entry. Across 192 cases, C and original bytes agree on the entire 1 MiB RAM, all 14 CPU fields, and every reached far-call entry frame. Cases vary the gate byte over `00,01,02,17,7F,80,FE,FF`, six incoming `AH` values, and four DS/SS/stack/loaded-segment layouts.

Four DOSBox-X normal-core synthetic far-entry captures execute the pinned wrapper and loaded stub for gates `00,01,80,FF`. Native execution matches original-byte Unicorn on all 14 CPU fields and 52 stack bytes. Ignored capture products stay under `resources/u38-317a-parity/native/`; capture SHA-256 `d92a6c42acefc6b6209f8134a8348269b85d36458f22036fbb8c6fc1f20158fc`. The focused strict CTest passes C11 `-Wall -Wextra -Wpedantic -Werror` with ASan/UBSan.

These are synthetic frames, not a naturally reached item-137 dispatch. The deterministic stub checks the wrapper ABI and return behavior; it does not establish real U9E removal side effects. An indirect item-137 table dispatch and any strict game-count decision remain separate review work.
