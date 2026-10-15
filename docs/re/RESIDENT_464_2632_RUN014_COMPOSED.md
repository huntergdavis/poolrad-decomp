# Resident 0464:+2632 saved run014 composition

## Scope and provenance

The ignored `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The original resident body at file `0x06CAB..0x07086`, corresponding to
`0464:+2632..+2A0D`, is 987 bytes, SHA-256
`ee35f98f513d5144e92e55e2afafbe176bba9ab65acc905ea151c19c5a9d713a`.
The body includes unvisited branches. This C work composes **one reached
saved-slot-A run014 path**, from loaded `0C88:2632` through its return to
overlay `2D14:01C3`. It is a bounded model, not a full translation or a
strict full-C count increment.

Before implementation, two bounded `deja` searches for the saved-slot-A
post-input route and for `RES_0464:2632 run014` timed out without an indexed
result. Reused local research: the earlier `RESIDENT_464_2632_PRECALL`
prefix on p5 commit `aa149f7`; the independently audited Pascal
`0AF8:0A3F` Move body on p5 branch `codex/runtime-move-a3f`; and the
numeric native frames in pD's
`docs/re/U3BD_ENTRY5_05FD_NATIVE_FRAME.md` at local commit `3e941e1`.
The complete original `+2632..+2A0D` span and selected calls were
independently hashed from the ignored executable for this composition.

## Caller frame and reached effects

The overlay caller at `2D14:01BE` enters loaded `0C88:2632` with
`SS:SP=1D16:3E82`, return `2D14:01C3`, destination object
`3DE4:0008`, source `14A0:260A`, selection mask `0x000F`, one row,
11 four-byte elements, row 59, and horizontal index 2. The object header
gives width 21 and height 168. The entry CPU tuple and 640 KiB RAM image
are from the same run014 capture; entry RAM SHA-256 is
`7a4593ce6f8043fda34209915e84f1f936bdb6dccbc9e72d2fb24a20a5c2cd2a`.

The reached path:

1. Copies eight source bytes to its stack local through original runtime
   `0AF8:0A3F`; the original 25-byte resident prefix
   `+2632..+264B` hashes to
   `89cf824e68b75add1e89474bcfacfa7463affb971ed885df123407f1ad5c5e70`.
2. Takes the positive-X, nonnegative-row, video-state-1 branch to
   `+28E0`. Its 60-byte `+28E0..+291C` prelude hashes to
   `f08efb35be49b51a38899a5a94e7f1bb0001691731517c634777d5c10b759716`.
   The runtime fill at `0AF8:16A8` zeros the four-byte local mask, then
   four table tests set the selected mask bytes.
3. Marks dirty row 59 and updates `DS:4A28`, `DS:4AD0`,
   `DS:4C20`, and `DS:4D70`. Eleven far calls to original runtime
   `0AF8:1685` copy four bytes each to object
   `3DE4:1383..13AE`. The reached runtime body compares source and
   destination offsets before choosing copy direction; this comparison
   owns FLAGS at the first `+29CB` return.
4. Takes the equal outer-counter branch at `+2A02`, then
   `RETF 0x12` at `+2A0A` to `2D14:01C3`. The six-byte epilogue
   `+2A07..+2A0D` hashes to
   `baf3c47a5c372266c40c50894f4cad9e7b96ee09fe943347feb9b6079cd4848b`.

From native entry to native return, exactly 57 RAM bytes change: 44
object bytes, six dirty/extent metadata bytes, and seven stack/frame
bytes. The final 640 KiB RAM SHA-256 is
`722cac9dd2a9c8e002649255183878a5687054ed65f19105f09d228ea0cf154c`.

## Exact comparison

The C composition begins with the **unchanged private run014 entry image**.
It compares full 640 KiB RAM and all 14 recorded CPU fields, including
FLAGS, at each of these instruction boundaries:

`+2632`, `+2646`, `+264B`, `+28E0`, `+291C`,
`+2921`, `+294D`, `+2999`, `+29C6`, `+29CB`,
`+29D7`, `+2A02`, `+2A07`, and overlay `+01C3`.

Result: **14/14 checkpoints exact**, with zero RAM and zero CPU differences.
The original call sites `+291C..+2921` and `+29C6..+29CB`
independently hash to
`adc5985dcb761563534658cb03a15d9afdf2305d7e2b82f800875d921f91ee3a`
and
`5117a3050a3452edeab0a91acc4044075e683817ae3b6dba422197bc6d972eef`.
This compares same-run images only; it does not pair run013 with run014.

The optional private comparison mode of
`tests/recovered/test_resident_464_2632_run014.c` accepts the absolute
entry and return image paths as arguments. Normal CTest runs synthetic
valid and unsupported paths without requiring game media. The public
source contains no original executable bytes, RAM dumps, or pixels.

## Boundary

The preflight rejects other video states, signed/negative or out-of-range
row shapes, missing selected table entries, and unmeasured extent branches
before any mutation. That host guard is outside the DOS instruction path.
Other `+2632` branches, including the earlier first-call object writes,
remain unmodeled. Existing `+08D4` metadata and `+02A5` mode-one helpers
are separate routines; neither is a replacement for this resident body.
