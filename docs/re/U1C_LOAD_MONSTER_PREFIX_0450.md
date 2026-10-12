# Unit 0x1c +0x0450: LOAD MONSTER (0x0B) entry-prefix model

A bounded model of **only the entry prefix** of opcode `0x0B` (LOAD MONSTER), unit `0x1c` `+0x0450` (whole routine `+0x0450..+0x0816`, file `0x1196..0x155C`, 967 bytes, 306
instructions, SHA-256 `bf1903765b12e98cfc5749630f31e2bedcabc607346255cdfae6b641759716cd`, verified), from the start through the return of the first call to unit `0x25` entry 5
(`+0x04AA`, `0x2B:0x39`, [U25_ENTRY5_0411.md](U25_ENTRY5_0411.md)). Sources: the static map [U1C_LOAD_MONSTER_0450_MAP.md](U1C_LOAD_MONSTER_0450_MAP.md) and the initializer map
[U6F_ENTRY6_0EC4_MAP.md](U6F_ENTRY6_0EC4_MAP.md). `deja` timed out (no hit); the earlier notes were reused. Only offsets, lengths, hashes and instruction descriptions are published.
**Not a model of the routine:** nothing after `+0x04AF` is modelled; no object contents, no count, no party links and no final PC are invented. No strict full-C, route, identity,
DOS/native or one-Up claim.

## What is modelled, in the original order

| Range | Effect | Model |
| --- | --- | --- |
| `+0x0457..+0x0461` | `saved := far [0x5D92]:[0x5D94]` (restored only at `+0x0806`) | read-only copy in the report |
| `+0x0468..+0x046B` | loader(3) (`0x2B:0x2A`) | lane-A loader; BOUNDS/ARGUMENT stop |
| `+0x0470..+0x0477` | `cmp byte [0x4435], 0x3F; jb +0x047A` else `jmp +0x0813` | **early return when `[0x4435] >= 0x3F`**: a complete path of the original (`mov sp,bp; pop bp; retf`), with only the loader's PC advance done, **no slot read, no wrapper call, outputs untouched** |
| `+0x048A..+0x0492` | `id := AL` of slot value(1) (`0x2B:0x25`) | lane-A slot value; a class-1 load-hook **HOOK stops here, before the wrapper**; BOUNDS/UNRESOLVED stop |
| `+0x0495..+0x04AA` | `0x2B:0x39` (unit `0x25` entry 5) with `mov al,[bp-1]; push ax` and three far variables A, B, C (the caller's `[bp-0x10]`, `[bp-0x142]`, `[bp-0x14E]`) | the merged wrapper model with its explicit `GetMem` and `0x7C:0x3E` initializer hooks; the model stops at its return (`+0x04AF`) |

Slot values 2 and 3 are read **after** the wrapper call in the original (`+0x04E6`, `+0x04FB`), so they are not part of the prefix. A, B and C start as explicit host stack residue (the
wrapper zeroes A first and writes B and C only when the initializer and both field reads succeed); the report shows what the wrapper left.

## API

`pr_u1c_op_load_monster_prefix` (`overlay_u1c_load_monster_prefix_0450.{h,c}`; CMake edit in this PR, coordinated per the PM): statuses PREFIX_DONE (the wrapper returned OK; the rest of the routine is
unmodelled), CAP_RETURN, CORE (loader or slot failure/HOOK), WRAPPER (a non-OK wrapper status, stopping at the unresolved record initializer or `GetMem`) and ARGUMENT. The initializer word the wrapper pushes is
`(GetMem offset & 0xFF00) | id low byte` (the wrapper's documented AX behavior), passed through unchanged.

Tests (`test_overlay_u1c_load_monster_prefix_0450.c`) are fully synthetic: the normal prefix and its exact PC; the early cap for `[0x4435]` = 0x3F, 0x40, 0x41, 0x42, 0x7F, 0xBC, 0xF9 (no slot read of a hooking operand,
no wrapper call, outputs untouched) with 0x3E running the body; a class-1 HOOK at slot value(1) (`0x6B18`) stopping before the wrapper with outputs untouched and a plain class-1 id (`0x6E80`) passing; the full initializer
word for a word-valued slot 1 and all 256 low bytes; the unsupported class-4 and truncated-operand stops; the untouched-output behavior for a failing `GetMem`, a failing or unbound initializer and an unmapped object; and
argument guards.

## Private supplied-state observation (documented, not committed)

Hash-pinned slot A, the supplied route through CLEARMONSTERS (explicit host inputs, none from DOS), then LOAD MONSTER at `0xB1B0` with the prefix bound and **fabricated** `GetMem`/initializer hooks (no saved data): `[0x4435] = 0`
so the body runs; slot value(1) is the plain class-1 word `0x6E80` with value 0, so `id = 0`; the wrapper is called once (fake `GetMem` then fake initializer, id word `0x0000` because the fake offset is `0x0010`); the PC
after loader(3) is `0xB1BA`. The model then stops at the wrapper return; the run reports the handler as not complete. This says nothing about the real record content, count, party links or the final PC.
