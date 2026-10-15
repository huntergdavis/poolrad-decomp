# Saved-A Close / IOResult selector return

The disjoint adapter executes actual resident 0802:0324 Close setup,
loaded at 1026, through SYSTEM Close 1491 to its first INT21 at 14A6.
A separate C execution starts at the actual native post-DOS 14A8 input,
executes the Close RETF4, actual IOResult 04A2 body, DS:49CE writer, and
selector RETF4 to the reached caller 1026:0292. DOS is an explicit gap:
no callback, supplied registers/flags, RAM journal, or in-flight snapshot
replacement connects these two fragments.

Base is main 217e809448be096fcc48b2413714856f5b165284. Six new files only;
old probe runtime gates and all shared integration/scoreboard files stay
unchanged. No full-C, function identity/count, or playable credit is claimed.

## Provenance and code pins

Both bounded prior-session queries ("Poolrad SYSTEM 1491 Close 04A2 IOResult
032A 032F 49CE" and "Poolrad 1491 IOResult") expired without reusable output.
Repository leads reused and rechecked are the
[existing probe ABI](SLOT_A_FILE_PROBE_02EB_ABI.md),
[selector notes](RESIDENT_DAX_SELECTOR_02EB.md),
[nested-open notes](RESIDENT_DAX_NESTED_OPEN.md), and
[approved Reset evidence at exact 6756d75](https://github.com/huntergdavis/poolrad-decomp/blob/6756d751da3e4ee2046c957a519426a70936ee22/docs/re/SYSTEM_RESET_1410_ABI.md).
The preceding Reset branch is unmerged; this new API does not change or
compose its DOS boundary. Existing slot_a_file_probe_02eb_abi.c supplies
unchanged resident 0324 setup and 0334..0353 CPU steps. Native audit readers,
CPU layout/comparer and original-instruction Unicorn stepping reuse
audit_slot_a_delete_abi_map.py and audit_slot_a_string_0356_abi.py.

A fresh original/native saved-A session ran through the actual C26 loader
and actual Reset, stopping at 0324. All CPU/RAM checkpoints were captured
without guest edits, and the native log was frozen before implementing C.
There are 35 native states and 33 post-input checkpoints across two fragments.
A failed initial launch had no interactive debugger input; it was stopped,
then a new original session was launched with a PTY. Its separate startup
log is excluded from the audited transcript. Saves/executables are pinned
again; no game-derived content is committed. Private inputs and captures
remain under resources/oracle-runs/slot-a-close-ioresult-20261014.

| Original artifact | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| SYSTEM 1491..14C4, including its near helper | 52 | 83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858 |
| SYSTEM 04A2..04A8 IOResult | 7 | d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b |
| Original resident 0324..0355 | 50 | ec66eb474eafa862b715fe8ddbc47161fea211fe275d0807ffe7e7e9ea876208 |
| Native raw transcript | local | cfaf06b175e54aeb3e2fbc3f0f3099baac3a409059f20aa0c3c1dfaa47c545d6 |

All snapshots contain the pinned SYSTEM bodies and resident tail with exactly
two AF8-to-131C call-segment relocations. SYSTEM 14C5 begins a different body;
IOResult 04A9 begins a different entry. Neither is supported by this adapter.
The audited original input hashes and every complete RAM hash are required.

## Measured ABI and DOS gap

At 0324: SS=ES=1D16, DS=14A0, SP=32D0, BP=337A, AX=0080, BX=32C4,
CX=0000, DX=3300, SI=3368, DI=32D0, FLAGS=7246. The temporary 128-byte record
is 1D16:32D0, with handle 0006, signature D7B3 and record size 0080.
The actual 032A far call enters 131C:1491 at SP=32C8 with stack words
[032F,1026,32D0,1D16]. MOV BX,SP and LES SS:[BX+4] load that pointer.
The near CALL 1497->14B6 pushes 149A at SP=32C6; the helper checks signature D7B3
and returns via a real near RET. Close loads handle 6, compares it with 4,
and builds AX=3E80 while preserving the input AL 80.

~~~text
Before DOS: CS:IP=131C:14A6 DS=14A0 ES=SS=1D16 FS=GS=0000
SP=32C8 BP=337A AX=3E80 BX=0006 CX=0000 DX=3300
SI=3368 DI=32D0 FLAGS=7202
~~~

The typed stop reports original segment AF8, call 14A6, return 14A8, and
actual AX/BX. Repeated calls preserve CPU and all 655360 RAM bytes.
Original DOS Close returned AX=3E01, carry clear, IP 14A8, with all other
recorded CPU fields unchanged. It changed 25 conventional RAM bytes, leaving
the 128-byte record unchanged. Those 25 bytes and DOS CPU changes are measured
native effects, excluded from C parity.

The independently seeded 14A8 fragment marks the record signature D7B0,
retaining handle 0006, size 0080 and all other bytes. Close's RETF4 consumes
eight stack bytes and returns 1026:032F with SP=32D0. The actual IOResult
far call enters 131C:04A2 at SP=32CC with [0334,1026] and no explicit arguments.
IOResult XORs AX, exchanges the full word at DS:4426 with AX, and RETFs.
Native DS:4426 was zero, so its complete RAM is unchanged from 04A2 to 0334.

At 0334 the full AX word is stored at DS:49CE; native zero remains zero.
The nonempty local Pascal name (length 23) selects AL 1, stores and reloads
that local result byte, restores SP/BP, and executes real selector RETF4.
Actual caller 0292 has SP=3384, BP=33D8, AX=0001, BX=0006, CX=0000, DX=3300,
SI=3368, DI=32D0, DS=14A0, ES=SS=1D16, FLAGS=7206. This adapter stops there
before executing any 025D caller continuation.

## Verification and adversarial review

~~~sh
bash tests/replay/check_slot_a_close_ioresult_abi.sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_close_ioresult_abi.sh \
  resources/oracle-runs/slot-a-close-ioresult-20261014
~~~

The complete audit passes 649 full CPU/RAM instruction comparisons:
317 new instructions and 332 unchanged-helper comparisons. The native prefix
contributes 15 comparisons (12 new), and the independent native suffix 18
(seven new), with all 33 post-input native checkpoints matching exactly.
There is no flags mask. Each case is bounded at 100 instructions / 10s;
the whole audit at 120s plus 5s termination allowance. New C/test compile
with C11 Wall/Wextra/Wpedantic/Wconversion/Werror. Existing helper objects
use their established strict warning flags, with no shared warning fixes.

Controlled cases start from independently copied inputs: three invalid
signatures (error 0067), reserved handles 0..4 avoiding DOS, four larger handles
stopping before DOS, full flag variants, IOResult words 0000/0001/0100/8000/FFFF
with empty/nonempty local names, three synthetic independent Close error tails,
both far-return stack adjustments wrapping from SP=FFFC, and a guest alias
where XCHG clears its own return IP before actual RETF reaches unsupported 0000.
The latter checks original write/read order and blocks at that unsupported PC.
Synthetic error-tail inputs do not establish measured DOS failure behavior.
Strict tests also retain old Close/IOResult runtime gates, verify real nested
call/return frames, whole-word XCHG, typed DOS stop, null arguments and atomic
memory/segment bounds. The complete strict and differential suites also pass ASan/UBSan, including
all 649 comparisons and 33 native checkpoints.

- target_function: reached Close 1491/IOResult 04A2 CPU fragments, DS:49CE
  writer and actual nested selector return 0292.
- source_candidates: slot_a_close_ioresult_abi.c plus unchanged resident
  steps in slot_a_file_probe_02eb_abi.c.
- signals: pinned original/loaded code, same-run native CPU/RAM, actual
  far/near frames, record/error/global writes, full flags, branch/alias tests.
- score: 4/5 ordinal evidence strength for these fragments; not a probability.
- verdict: match, bounded to the stated fragments; complete Close/DOS unclear.
- review_notes: native DOS errors, handle effects, Create/Read/Write, earlier
  Reset/Assign composition, caller 0292 continuation, and continuous saved-A C
  execution remain unsupported. No function/library ownership credit follows.
