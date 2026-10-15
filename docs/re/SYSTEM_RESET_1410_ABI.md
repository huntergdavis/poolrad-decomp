# Saved-A SYSTEM Reset 1410 / DOS boundary

This disjoint replay adapter starts at the actual resident 0802:031F call,
loaded at 1026:031F, and executes the Reset CPU prefix to its first INT21.
A separate audit starts from the actual native post-INT state at 131C:1458
and executes the success tail through real RETF6 to 1026:0324. DOS remains
an explicit unsupported dependency: the adapter supplies no service callback,
AX/CF result, handle, RAM journal, or continuous C execution across INT21.

Base: main 217e809448be096fcc48b2413714856f5b165284. Scope is six new replay
source/header/test/runner/audit/evidence files. No shared integration, original
media, identity/function count, or playable-route credit changes.

## Provenance and reuse

The required prior-session query, deja query "Poolrad SYSTEM 1410 Reset DOS 3D 031F 0324 file record", completed with no matches. Repository leads were
[the nested-open notes](RESIDENT_DAX_NESTED_OPEN.md),
[the selector notes](RESIDENT_DAX_SELECTOR_02EB.md), and
[the existing probe boundary](SLOT_A_FILE_PROBE_02EB_ABI.md).
The immediately preceding approved [Assign evidence at exact 1a79ca7](https://github.com/huntergdavis/poolrad-decomp/blob/1a79ca7e4dd4385b054982007a54f80b2212ba0d/docs/re/SLOT_A_OPEN_025D_ASSIGN_ABI.md)
identified the pre-031F input. That branch is unmerged; this adapter does not
change or silently bridge the existing Assign/probe API. The audit reuses the
CPU layout, exact CPU comparer, pinned media reader, and Unicorn single-step
helper from audit_slot_a_string_0356_abi.py and audit_slot_a_delete_abi_map.py.

The original/native run finished and its log/RAM were frozen before writing
this C implementation. All captures, original files, screen images, and builds
remain ignored under resources/oracle-runs/slot-a-reset-1410-dos-abi-20261014.
The public audit contains hashes and addresses only. The one initial EV row
was repeated after a missing-python command retry; both rows must be identical.
The audit checks exactly 28 EV rows representing 27 unique saved CPU/RAM states,
then explicitly drops that duplicate row. There are 25 post-input native
checkpoints across the two independent fragments.

Pinned input SHA256:

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| native.log | local | 389b432f09fdaaadf1430e48ae5f340e1bb85a0e84b606577ed38f143bf9a02c |
| Reset1410..1474 | 101 | 18b5680c7d5ea7bfdee3902b7a11279f62216c07fbff9c692fb8820363b481f0 |

The 101-byte body is START C086..C0EA (SYSTEM file base AC76). Every snapshot
must contain the exact loaded 131C body and the resident 031F far call with
its original AF8 segment relocated to131C. Address1475 begins another routine
and is excluded. Alternate Create entry1419 is not supported.

## Actual ABI

Pre-call031F has SS=1D16, SP=32CA, BP=337A, DS=14A0, ES=1D16,
AX=0080, DI=32D0, FLAGS=7246. Stack words are
[0080,32D0,1D16]: record size 128 and far pointer1D16:32D0.
The actual far call pushes [0324,1026], leaving SP32C6; Reset PUSH BP
leaves BP=SP=32C4. Its LES at 1422 reads the file pointer at BP+8.
The existing record signature is D7B0, and the raw filename length is23.
Its bytes remain private.

Reset loads FileMode 2 from DS:442C, builds AX 3D02, and selects the assigned
record branch. At the pre-INT 1456 gate:

~~~text
CS:IP=131C:1456 DS=ES=SS=1D16 FS=GS=0000
SP=32C2 BP=32C4 AX=3D02 BX=32C4 CX=0000 DX=3300
SI=3368 DI=32D0 FLAGS=7246
~~~

DS:DX points to the record's raw NUL-terminated name at offset30h. The gate
reports the actual AX/DS/DX and original segment AF8, call 1456, return 1458.
CPU and all 655360 RAM bytes remain unchanged on repeated blocked calls.

The original DOS interrupt returned AX 0006, carry clear, and IP 1458. Every
other recorded CPU field was unchanged. Conventional RAM changed at 18 bytes;
the assigned 128-byte record was unchanged during the interrupt. These 18 bytes
and the DOS CPU result are measured native effects, excluded from C parity.

The independently seeded 1458 fragment pops the real saved DS 14A0, takes JNC,
writes record signature D7B3 and handle 0006, loads size 0080 from BP+6, and writes
that size at record+4. The remaining 122 record bytes are unchanged. POP BP
restores 337A; RETF6 consumes 10 bytes including its far return frame and six
argument bytes. Caller 0324 has SP 32D0, AX 0080, DS 14A0, DX 3300, FLAGS 7246.
The adapter blocks there before executing any Close or IOResult caller work.

## Verification and bounded verdict

Run the media-free strict tests:

~~~sh
bash tests/replay/check_system_reset_1410_abi.sh
~~~

Run the private audit with Unicorn2.1.4 and assertions enabled:

~~~sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_system_reset_1410_abi.sh \
  resources/oracle-runs/slot-a-reset-1410-dos-abi-20261014
~~~

The expected result is 316 complete CPU/RAM instruction comparisons: 17 in
the native pre-DOS fragment, eight in the independently seeded native suffix,
and 291 controlled comparisons. All 25 post-input native checkpoints also match
exactly; flags are not masked. Each case is bounded at 100 instructions / 10s,
the process at 120s plus 5s termination allowance. Strict C11 compilation uses
Wall/Wextra/Wpedantic/Wconversion/Werror. The complete strict and differential suites also pass under ASan/UBSan,
including all 316 comparisons and 25 native checkpoints.

Controlled cases independently copy their initial native image: invalid record
signatures0/D7B1, already-open D7B3 reaching the typed Close 1491 gate before
its CALL at 1442, four FileMode values under three full flag patterns, an empty
name taking the no-DOS path, three isolated error-tail AX/CF inputs, and a
RETF6 frame at SP FFFC exercising stack adjustment wrap. Controlled error
inputs are synthetic CPU-tail checks, not measured DOS errors or injected
results in an in-flight C execution. The strict tests additionally check
out-of-range/segment-crossing spans, null arguments, unchanged blocked state,
unsupported Create, and actual far-call/return frame sizes.

- target_function: reached SYSTEM 1410 Reset CPU fragments and resident 031F ABI.
- source_candidates: src/replay/system_reset_1410_abi.c only.
- signals: pinned original/loaded code, ordered full CPU/RAM comparison,
  actual native pre/post-INT states, record stores, call/return frames,
  negative branch tests, and immutable typed dependencies.
- score: 4/5 ordinal evidence strength for these fragments; not a probability.
- verdict: match, bounded to the stated CPU fragments; complete Reset/DOS unclear.
- review_notes: native error paths and existing-open Close effects remain
  unmeasured. Close 1491, post-Close 1445, Create 1419, all DOS effects, later
  caller 0324 work, and continuous saved-A C execution remain unsupported.
  The synthetic error tail does not establish a real DOS failure contract.
