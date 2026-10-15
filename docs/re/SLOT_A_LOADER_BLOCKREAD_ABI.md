# Actual loader C2B / runtime 14FB ABI

The bounded adapter executes three independent CPU fragments: actual loader
C2B to runtime 14FB call C44; independently seeded native after-read C49 to
Close C4F; independently seeded native after-Close C54 through eight word
stores to sidecar boundary C8F. Both runtime calls block before their frame
pushes. No read/Close/DOS callback, result registers, journal or in-flight
state replacement connects these fragments.

Base is main 217e809448be096fcc48b2413714856f5b165284. Six new replay files
only. Caller 0292, sidecar setup, allocation, printer, shared integration and
all identity/count/playable claims remain unchanged.

## Provenance and original evidence

Bounded prior-session query "Poolrad loader C2B 0C44 14FB BlockRead C49 C4F"
expired without reusable output. Reused leads, rechecked against original and
fresh native state, are the [loader continuation map](U6F_PARTY_CONTINUATIONS.md),
[earlier runtime 14FB context](RESIDENT_DAX_FILE_OPEN.md),
[approved caller0292 return](https://github.com/huntergdavis/poolrad-decomp/blob/d6e702794235ddc2b4d07136ae20b7f0d8e459b6/docs/re/SLOT_A_OPEN_0292_CONTINUATION.md), and
[approved sidecar input boundary](https://github.com/huntergdavis/poolrad-decomp/blob/00755c3e98a966b56a39d0dbf0b10d173c1fac7e/docs/re/SLOT_A_LOADER_SIDECAR_ABI.md).
Those worker branches are unmerged; this adapter has no C dependency on them.
The private audit reuses the existing 0356/delete CPU layout, exact comparer,
pinned-media reader and original-instruction Unicorn helpers.

A fresh untouched original saved-A session reached C2B through actual C26
and original 025D effects. All 38 native CPU/full-RAM states were frozen before
C implementation: three fragment inputs, 31 post-input C checkpoints, two
native CALL-entry states and two DOS states. Original-CALL-only emulator
checks are separate from C counts. The DOS states are measured evidence,
not C outputs. All original inputs, payloads, log, RAM, screenshots and
builds remain ignored under
resources/oracle-runs/slot-a-loader-blockread-abi-20261014.

## Actual runtime frames and DOS read

C2B enters with SP=33EC, BP=37A6, AX=0001, DS=14A0, ES=SS=1D16. C saves AL
at BP-1B9, prepares file record 1D16:3626, buffer 449A:0008, requested count
011D (285), and actual-count word 1D16:3624. The file record's native size
is 1, as established by the preceding open. At C44, SP=33DE holds:

~~~text
[3624,1D16,011D,0008,449A,3626,1D16]
~~~

The original far CALL enters 131C:14FB at SP=33DA with [C49,2A8E] preceding
those seven argument words. The entry fingerprint begins at original START
file C171, SYSTEM file base AC76; it is checked as a 32-byte fingerprint, not a
complete routine translation. The C adapter stops before this CALL.

Original runtime execution reached INT21 at 131C:1529 with AX=3F1D, BX=0006,
CX=011D, DS:DX=449A:0008, ES=1D16, SS=1D16, SP=33D4, BP=33D8, FLAGS=7202.
The real DOS interrupt returned AX=011D, carry clear, IP=152B; other recorded
CPU fields were unchanged. Conventional RAM changed at 145 bytes, 126 within
the 285-byte buffer and 19 outside it. This is 285 returned bytes versus 126 net
buffer-byte differences, not a claim that only 126 bytes were read. These
native DOS CPU/RAM effects are excluded from C parity.

At native C49 the actual-count word is 011D, SP=33EC, BP=37A6, AX=011D,
DS=14A0, ES=1D16, FLAGS=7206. Its independent C fragment computes the same
file-record pointer, pushes SS/DI, and stops at C4F. Original Close entry
131C:1491 has SP=33E4 and [C54,2A8E,3626,1D16]. Close effects remain unknown
C dependencies; the independent C54 input already includes their native
result (AX=3E01, FLAGS=7202).

The C54 fragment repeatedly reloads the buffer far pointer at SS:[BP+A],
XORs AX, and stores words at offsets 104,106,C8,CA,7F,81,108,10A in that
order. Eight stores write 16 bytes; this natural image has eight net changed
bytes because some were already zero. C stops at C8F with AX=0000,
ES=449A, DI=0008, SP=33EC, BP=37A6, DS=14A0, FLAGS=7246. No sidecar instruction
executes and no meaning beyond the measured word fields is assigned.

## Pins and verification

| Original artifact | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original unit6F C2B..C8E | 100 | 72ce2055c7a265398578b903043b5b924402632b06c457c48c915f085122f529 |
| SYSTEM14FB entry fingerprint | 32 | ff0660891ecb3fd6c09cb7a346f640f29b7b5f294ecfd918cb90e07b02fb1b16 |
| Native raw transcript | local | e5bc8ffffb93343bc6fa0f9f7846d7459a223638e1fee1ac274bf0bff77966a5 |

Every native image verifies the original/loaded 100-byte loader span with its
two AF8-to-131C call-segment relocations, the 14FB entry fingerprint, original
INT21 at 1529, and the 52-byte Close body including its shared near helper.
Full media and every complete RAM image are hash-locked. No runtime identity
or implementation credit follows from those fingerprints.

~~~sh
bash tests/replay/check_slot_a_loader_blockread_abi.sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
 bash tests/replay/check_slot_a_loader_blockread_abi.sh \
 resources/oracle-runs/slot-a-loader-blockread-abi-20261014
~~~

The audit passes 307 complete CPU/RAM C instruction comparisons, including
31 natural comparisons (12 + 3 + 16) and all 31 post-input native checkpoints.
Two original-CALL-only full-state frame comparisons pass separately and are
excluded from C counts. The two native DOS states and 145-byte delta are
excluded from C effects. All defined CPU fields and all 655360 conventional
RAM bytes match after every C instruction, with no flag masks.

Controlled cases independently copy native inputs: raw AL/high-byte AX values
under three full flag patterns; zeroing nonzero word fields at native and
alternate pointers; effective-offset wrap; and a guest alias where the first
word stores clear their own argument pointer. Repeated LES then reads the
new pointer before subsequent stores, preserving original read/write order.
An independent mid-store AX=ABCD case checks whole-word MOV semantics; other
cases check returned-AX independence in Close setup and an alternate read
buffer pointer. No synthetic case installs file or DOS results into a running
C prefix. Strict tests also check unchanged runtime/sidecar gates, null
arguments, segment/RAM bounds, full-word stores, pointer reload and flags.

Cases are bounded at 100 instructions / 10s; the process at 120s plus 5s kill
allowance. Strict C11 Wall/Wextra/Wpedantic/Wconversion/Werror and the
complete strict+differential ASan/UBSan suites pass, including all 307 C
comparisons and both separate frame checks.

- target_function: actual loader C2B read argument setup, independent native
  after-read C49 Close setup, and native after-Close C54 word clears to C8F.
- source_candidates: slot_a_loader_blockread_abi.c only.
- signals: pinned original/loaded code, fresh native frames, real DOS input/
  output observations, exact CPU/RAM checkpoints, word-store order, flags,
  pointer wrap and argument-storage alias controls.
- score: 4/5 ordinal evidence strength for these fragments; not probability.
- verdict: match, bounded to the stated CPU fragments; complete read/loader unclear.
- review_notes: file/Close/DOS effects remain explicit gaps. No short-read,
  EOF or error contract, byte-decoding meaning, allocation, printer, later
  sidecar occurrence, full-C ownership or playable saved-A parity is claimed.
