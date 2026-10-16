# U6F +01C3 caller prefix to the +04F7 boundary

The local C stepper implements loaded `2A8E:01C3..01E3`, including the
actual far calls to SYSTEM `131C:0B0B` and `131C:0B98`. It stops before
the CALL at `01E3`; it supplies no +04F7 text-record initialization, Assign,
file open, DOS/UI effect, retry/list continuation or outer return.

The caller has 32 bytes and 14 instruction starts. Its pushed words remain
in live guest RAM. The two helpers are the existing, unchanged adapters in
[slot_a_saved_copy_1965.c](../../src/replay/slot_a_saved_copy_1965.c) and
[system_b98_abi.c](../../src/replay/system_b98_abi.c). Their real far frames,
REP forward-copy alias effects, saturated Pascal append, registers and flags
are reused. A modified helper return frame commits its actual transfer;
the caller never forces the expected continuation.

The API retains the project's complete 16-word CPU structure, including
FS/GS. It executes one instruction per step, with inherited atomic REP.
Each supported operand must fit its segment and supplied RAM without A20
wrap; RAM may not exceed 1 MiB. State and RAM must be exclusive and disjoint.
Undefined logic AF follows the existing adapters' normalization to zero.
Unsupported spans, PCs and inherited unsupported DF states reject without
changing CPU or RAM for that step. No automatic code-identity or prior-entry
provenance is inferred from a caller-supplied PC.

## Source identity and provenance

| Original input | Bytes | SHA-256 |
| --- | ---: | --- |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| U6F `[01C3,01E3)` | 32 | `31a9993b6b58cee6e88c2ac0796d3b5dad94454e2f33a8e231bea06b439ae108` |
| SYSTEM `[0B0B,0B25)` | 26 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| SYSTEM `[0B98,0BC4)` | 44 | `c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64` |

Reused static evidence is the ignored `resources/u6f-next-readonly-i7909/`
packet: `A_SOURCE_PACKET.md`, `source-facts.json` and ownership records.
Explicit relative pG clearance i8003 was read and recorded before edits,
alongside p9 i7927, p5 i7928, pF i7929 and census i7932. PM i7978 and the
subsequent user handoff authorize this local bounded implementation.
The prior-session `deja` lookup timed out; no historical search result is
claimed or reused. Shared CMake, existing APIs and evidence ledgers are untouched.

## Focused validation

Run `bash tests/replay/check_u6f_01c3_text_record_prefix.sh` and repeat with
`--sanitize` for ASan/UBSan. The standalone runner uses strict C11,
`-Wall -Wextra -Wpedantic -Werror`. Synthetic cases check real frames and
retained arguments, empty/max/saturated append, nonzero FS/GS, boundary
idempotence, atomic rejection and modified return-frame transfers.

The separate `tests/replay/audit_u6f_01c3_text_record_prefix.py` compares the C
implementation against original instructions using pinned
[Unicorn 2.1.4](https://github.com/unicorn-engine/unicorn/releases/tag/2.1.4).
Eleven bounded cases passed comparisons of all 16 CPU words and every byte
of 1 MiB RAM: eight length/saturation cases and three forward-copy overlap
positions. Original inputs and all three code spans are hash-locked.
Only the two actual far-CALL segment fields are relocated from original
`0AF8` to loaded `131C`; instruction and branch bodies are unchanged.
Inputs are synthetic; the audit does not demonstrate original caller
reachability, arbitrary alias domains, asynchronous events or native parity.

Build the audit library in ignored resources, then provide private original
files explicitly (Python must be able to import Unicorn):

```sh
mkdir -p resources/build-u6f-01c3-oracle
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -shared -fPIC -Iinclude \
  src/replay/u6f_01c3_text_record_prefix.c src/replay/slot_a_saved_copy_1965.c \
  src/replay/slot_a_string_0356_abi.c src/replay/system_cbc_abi.c \
  src/replay/system_b98_abi.c src/recovered/slot_a_directory_fullstate.c \
  -o resources/build-u6f-01c3-oracle/libprefix.so
python3 tests/replay/audit_u6f_01c3_text_record_prefix.py \
  resources/DOS/POOLRAD/GAME.OVR resources/DOS/POOLRAD/START.EXE \
  resources/build-u6f-01c3-oracle/libprefix.so
```

No native emulator was started. This is a bounded caller/helper adapter,
not a whole-function implementation or new strict-credit census row.
