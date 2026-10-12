# Unit 0x9B:+0x0000 enclosing record routine

`pr_u9b_entry_zero_0000` translates the DOS v1.3
`0x9B:+0x0000..+0x01AE` routine and composes its three internal
near callees. The routine traverses eight record indices, updates
record fields through DS tables, and conditionally calls
`+0x01AF`, `+0x0255`, and `+0x0329`. The field meanings remain
unknown.

Before implementation, `deja "Poolrad u9B 0000 enclosing routine compose
01AF 0255 0329 native saved insertion"` found no exact match.
`deja "u9b 0000"` recovered the [runtime dispatch
research](U9B_RUNTIME_DISPATCH_0000.md). This work reuses its pinned
saved +0x0000 and +0x01AC snapshots, the [u6F transfer
map](U6F_U9B_TRANSFER_1574.md), and the reviewed
[+0x0255](U9B_RECORD_TABLE_0255.md) and
[+0x0329](U9B_RECORD_FIELDS_0329.md) full-state leaves.
The enclosing body and all callees were independently rechecked in
the ignored GAME.OVR image.

| Unit span | GAME file span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| `+0x0000..+0x01AE` | `0x2AA87..0x2AC35` | 431 / 159 | `260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71` |
| `+0x01AF..+0x0254` | `0x2AC36..0x2ACDB` | 166 / 49 | `5dd90fb8796e7aa15a770e4868aaceeee0a0886e0251a9fc7c10f3f32935652c` |
| `+0x0255..+0x0328` | `0x2ACDC..0x2ADAF` | 212 / 86 | `f19017361e6ac6197e38eeee7b70f438d29962a34675ff43903ac712444dd326` |
| `+0x0329..+0x045D` | `0x2ADB0..0x2AEE4` | 309 / 125 | `f5ff12924988b708edcf3756213532a2c5ba1b4860a5f63e11ffcd229297f3d6` |

START.EXE is SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The 159-instruction enclosing span has only three near calls:
`+0x00EC -> +0x01AF`, `+0x0191 -> +0x0255`, and
`+0x01A6 -> +0x0329`. All its branch targets stay inside the
enclosing span. No external call is required.

## Source-ordered path

The API begins after the far caller's CALL, with return IP/CS and far
record argument already in SS RAM. `+0x0000..+0x000E` pushes BP,
reserves eight bytes, and clears record `+0x2D`. Its outer index
`[BP-1]` runs from zero to seven. At each index it rereads the far
argument and record `+0x96+index`, caches the latter at `[BP-3]`,
and compares record `+0x2D` against DS table
`+0x3C7C + 11*index + signed(class)`. The table is recomputed on
the taken store path. It also conditionally raises signed record
`+0x73` and sets record `+0xA1` for index two.

At index zero, signed class greater than one enters a three-byte DS
table store to record `+0xB2..+0xB4`, then near-calls
`+0x01AF` at `+0x00EC`. A 56-index scan at
`+0x00EF..+0x013E` conditionally sets record `+0x32+index`.
At index five, signed class greater than one enters another
three-byte table store to record `+0xB5..+0xB7`. After index seven,
`+0x0191` always calls `+0x0255`. The signed
record `+0x9C` comparison at `+0x0197` calls
`+0x0329` at `+0x01A6` only when positive, then
`+0x01AC` performs `RETF 4`.

For each near call, the C model pushes the live argument segment,
argument offset, caller CS, and near return IP in that source order.
The `+0x0255` and `+0x0329` calls use their reviewed full
1 MiB RAM/register APIs. The existing `+0x01AF` public API
describes record effects through a segment view and cannot preserve
its caller-visible BP/SP, cached SS far pointer, ES:DI, flags, or
`RETF 4`. A local instruction-level implementation supplies those
effects without changing that API. All SS, DS, and ES reads and
writes use one A20-off 1 MiB RAM; LES and RETF operands read
contiguous physical bytes even across offset `FFFF`.

The C function returns `TRANSFER` with the reached RAM and CPU
state if an aliased near-call return frame changes CS:IP away from
the enclosing body's expected continuation. This makes the
diversion visible to a higher-level executor. It does not execute
arbitrary code at that altered address. The original can also loop
forever if an alias repeatedly changes a local loop index.

## Parity boundary

The hash-locked Unicorn audit executes original bytes for **112
deterministic synthetic cases**, including all three call sites,
index-zero and index-five tables, signed class gates, DS and SS
aliases, argument/return offset crossings, a changed top-level
return frame, and a diverted near return. For each case, every
byte of 1 MiB RAM and every register at the reached return or
transfer boundary matches the C model. The audit reports exact
near-call site counts.

The ignored saved slot-A capture starts at `2CCD:0000`; its
record `+0x96` at index zero is zero, so it skips
`+0x01AF`. After the reached `+0x0255` call,
record `+0x9C` is zero, so it skips `+0x0329`.
Starting from the pinned 640 KiB entry RAM and a frame-equivalent
CPU state, C matches **all 640 KiB** of the pinned native
`+0x01AC` RAM snapshot. The native capture pins RAM, not the
complete entry register set; its returned registers are compared
to original-byte Unicorn under the same selected initial registers.

```sh
cmake -S . -B resources/build-u9b-0000 -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u9b-0000
ctest --test-dir resources/build-u9b-0000 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u9b_0000_composition_audit.py --unicorn \
    --native-dir resources/oracle-runs/u9b-entry-zero-runtime-002
```

The public code and audit contain only addresses, counts, hashes, and
synthetic state. Original bytes, saved RAM, and game media remain
under ignored `resources/`. Project-wide strict and identity
counts are unchanged pending PM parity review.
