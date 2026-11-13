# Conditional run003 DOS35 composition

This diagnostic enters the existing resident C model at the captured pre-INT
`10D2:1386` and stops before `10D2:1388`. The caller explicitly supplies the
conditional DOS35 binding described in [SLOT_A_DOS35_RUN003.md](SLOT_A_DOS35_RUN003.md).
The model creates the interrupt frame and performs the handler return; the
binding supplies the DOS handler's observed stack and PSP writes and vector
result. The candidate PSP `0814` is an input hypothesis, not independently
established process ownership or executable provenance. The binding refuses
other PSP values and the model refuses an absent binding.

Run with ignored private capture data:

```sh
bash tests/replay/check_slot_a_dos35_composition_run003.sh PRIVATE_RUN003_DIR
```

The script checks the hashes of the two run003 RAM and CPU captures. The replay
starts with run003's 1386 RAM and CPU values and compares all 14 CPU words and
all 655,360 conventional RAM bytes to run003's 1388 capture. It reports zero
RAM differences. The post-capture is used only as expected output. This is an
observed-state diagnostic, not a continuous post-Up execution: the existing
continuous C prefix starts from run002, and a run003 same-invocation prefix
has not been established. The native comparison stops before the port output at 13A3. The normal 137C entry and its default refusal remain
unchanged.

The replay then continues that C-produced 1388 state, without reloading RAM or
CPU, through the resident vector-save and setup instructions to the second DOS
interrupt at 139E. It matches the same run003 139E capture in all 14 CPU fields
and all 655,360 conventional RAM bytes. The replay then supplies a separately guarded conditional AH=25 binding to cross the second interrupt and stops before 13A0. It matches all 14 CPU fields and all 655,360 conventional RAM bytes at that same-run stop; missing and unsupported bindings refuse. The handler uses the same supplied PSP hypothesis, not established ownership. The replay then continues through 13A0 and 13A1 to 13A3, matching all 14 CPU fields and all 655,360 conventional RAM bytes. The native comparison stops before the port output at 13A3.
This extends the bounded service replay, not the earlier run002-seeded post-Up
prefix. A prior independent full-image 1388-to-139E observation appears in
shared memory session `01a13101-709d-77e1-9fcb-2a6acbff0046`.

The prior independent observation of the second interrupt return and RAM capture is recorded in shared memory session `01a13101-709d-77e1-9fcb-2a6acbff0046`.

The prior independent bounded 13A0-to-13A3 replay is recorded in shared memory session `01a1281b-85e3-7e23-bdf6-1ea8af4d69de`.

The test then optionally supplies an abstract logging port callback and executes
only the first `OUT` at 13A3. It records one write of `B6` to port `43`,
advances IP to 13A5 with the other CPU fields and RAM unchanged under that
callback, and stops. An absent callback refuses at 13A3. This is a declared
instruction-level observation, not a PIT model: there is no same-run native
13A5 CPU/RAM comparison or PIT-state evidence, and no device or full-Up parity
is claimed. The port, value and instruction come from the recovered
[137C body](RES_08AE_VECTOR_137C_CPU.md); shared memory session
`01a13101-709d-77e1-9fcb-2a6acbff0046` mentions an earlier 13A5 observation
whose archive was not located.

A second optional abstract step starts from the C-produced 13A5 state. It
loads `AX=13B1`, records one subsequent `B1` write to port `40`, and stops at
13AA. A missing port callback refuses at 13A8 after the AX load. The logger
does not change RAM. This checks the order of the two declared writes, not PIT
state or native 13A5/13AA parity. The instruction sequence is documented in
[the recovered 137C body](RES_08AE_VECTOR_137C_CPU.md).

A third abstract step starts at the C-produced 13AA state. It executes
`MOV AL,AH`, records a third write `13` to port `40`, and stops at 13AE.
Without a port callback it refuses at 13AC after the register move. The test
checks all three writes in order and unchanged RAM under the logger. This does
not establish PIT state or native post-13A3 parity; the ordered instruction
sequence is documented in [the recovered 137C body](RES_08AE_VECTOR_137C_CPU.md).

After the three abstract port observations, the test continues through the
resident `POP ES; POP DS; RETF` sequence. The retained pre-port stack yields
ES=`3DA1`, DS=`14A0`, SP=`3DB6`, and return `10D2:00F4`; RAM remains unchanged
under the logging callbacks. This is a C instruction and stack check derived
from the 13A3 capture, not native post-port or PIT parity. The resident return
and the earlier observation are described in
[the recovered 137C body](RES_08AE_VECTOR_137C_CPU.md) and shared memory session
`01a13101-709d-77e1-9fcb-2a6acbff0046` respectively.

The test then resumes the existing dispatcher model at `00F4`. The retained
stack supplies argument 10; the modeled branch reaches `10D2:1742` with
argument 9 and return `0147`. The test checks all CPU fields and the full
conventional RAM image after applying the expected stack writes. The callee
is not executed. These expectations are derived from the captured pre-port
state and recovered instructions under the abstract port callbacks; they are
not a native post-port comparison or PIT-state evidence. See
[the recovered dispatcher](RES_08AE_DISPATCH_00DC_CPU.md). An earlier native
trace is mentioned in shared memory session
`01a13101-709d-77e1-9fcb-2a6acbff0046` but its post-port capture was not found.

The replay then composes the existing [1742 CPU model](RES_08AE_TABLE_1742_CPU.md)
on the C-produced argument-9 call and stops on its return at `10D2:0147`. It
checks all 14 CPU fields and the 655,360-byte RAM image against an 18-byte
**model-pinned** delta from the pre-call C state. These are regression checks,
not independent native observations. The input after 13A3 is conditional on
the abstract port callbacks; the test does not establish PIT state, native
post-port parity, the caller's 3492 return, or a complete Up frame. The next
step is the dispatcher continuation at 0147. A prior native 1742 observation
in shared memory session `01a13101-709d-77e1-9fcb-2a6acbff0046` concerns a
separate invocation and is not used as this replay's input or expected output.

The conditional replay resumes the existing dispatcher model at 0147 and
stops before `10D2:006C` with argument 10 and return `0165`. It checks CPU14
and conventional RAM against the modeled call-frame writes. The callee is
unexecuted. These checks depend on the abstract port callbacks and model-pinned
1742 return; they are not a native post-port comparison or a PIT-state claim.

The conditional replay enters the existing [006C delay model](RES_08AE_DELAY_006C_CPU.md)
with argument 10. With no time callback, it stops at `10D2:0000` after the
first time-call frame is pushed. The test checks CPU14 and the entire
conventional RAM image against the modeled prologue and frame. It supplies no
time values and does not execute the time service or claim a delay return.
The input remains conditional on the abstract port callbacks and model-pinned
1742 state; this is not native post-port parity or PIT-state evidence. The
next input is an explicit time-service binding and its environmental effects.

After the delay model has returned its `TIME` refusal, the replay explicitly
hands that preserved CPU and RAM state to the existing
[0000 time model](RES_08AE_TIME_0000_CPU.md). A declared GetTime callback
refuses without supplying a clock value; the time model stops in GetTime at
`1256:00A7`. This staged handoff does not mutate state inside the refusing
006C callback and does not resume 006C. CPU14 and conventional RAM are checked
against a 23-byte model-pinned stack delta, not a native post-port capture.
The next dependency is a supported DOS GetTime result and its environmental
side effects; PIT state, native post-port parity, and a complete Up frame remain
unverified.

A separate test-only branch supplies a declared synthetic ordered GetTime
sequence `00:00:00.00`, then `00:00:00.10`. Its DOS callback supplies CX:DX,
returns the declared continuation, preserves other CPU fields, and assumes DOS
makes no RAM writes. The time-model callback stages work on a copy and commits
only a complete return, preserving the 006C hook's no-mutation refusal contract.
With two supplied readings the C delay returns to `10D2:0165`; CPU14 and the
conventional RAM image match a 42-byte model-pinned delta. With only the first
reading, the next request refuses at the time boundary and an exhausted call
leaves CPU and RAM unchanged. These readings are caller-supplied examples, not
observed run003 time or evidence of DOS interrupt-frame effects. Native
post-port parity, PIT state and a complete Up frame remain unverified.

From the delay return produced by the two synthetic clock readings, the test
resumes the existing dispatcher at 0165 and stops before `10D2:17AE` with
argument zero and return `016C`. CPU14 and conventional RAM are checked
against the modeled call-frame writes. The callee is not executed. This is
conditional on the abstract clock and port bindings, not a native post-port
comparison or PIT-state claim. Earlier native dispatcher observations appear
in shared memory session `01a12b3e-db20-70e1-95f9-d3fe44b8d309` and are not
used as this replay's input or expected output.

The conditional replay then runs the existing [17AE CPU model](RES_08AE_TABLE_17AE_CPU.md)
with argument zero and stops at its `10D2:016C` return. CPU14 and conventional
RAM are checked against a nine-byte model-pinned delta. These regression
expectations are not independent native evidence: the input depends on the
abstract port callbacks and synthetic clock sequence. PIT state, native
post-port parity, the 3492 return, and a complete Up frame remain unverified.
The prior independent `17AE` observation is recorded in shared memory session
`01a12b3e-db20-70e1-95f9-d3fe44b8d309` and is not spliced into this replay.

From the conditional 17AE return, the test resumes the existing dispatcher at
016C and stops before the second `10D2:1742` call with argument zero and return
`0173`. CPU14 and conventional RAM are checked against the modeled call-frame
writes. The callee is not executed. This remains conditional on abstract port
callbacks and synthetic clock readings; no native post-port or PIT-state parity
is claimed. An earlier independent native observation is recorded in shared
memory session `01a12b3e-db20-70e1-95f9-d3fe44b8d309` and is not spliced into
this replay.

The conditional replay runs the existing 1742 model a second time with
argument zero and stops at its `10D2:0173` return. CPU14 and conventional RAM
are checked against a two-byte model-pinned delta. These are regression
expectations from the C-produced state, not native post-port evidence. The
input depends on abstract port callbacks and synthetic clock readings. PIT
state, the 3492 return, and a complete Up frame remain unverified. The earlier
independent native observation is in shared memory session
`01a12b3e-db20-70e1-95f9-d3fe44b8d309` and is not spliced into this replay.

From the conditional second 1742 return, the test resumes the dispatcher at
0173 and stops before `10D2:13B1` with return `0177`. CPU14 and conventional
RAM are checked against the modeled call-frame writes. The vector-restore
callee is unexecuted. This remains conditional on abstract port callbacks and
synthetic time; native post-port and PIT parity are not established. A prior
independent native stop appears in shared memory session
`01a12b3e-db20-70e1-95f9-d3fe44b8d309`, not as input to this replay.

The conditional replay enters the existing [13B1 restore model](RES_08AE_VECTOR_13B1_CPU.md)
without a DOS binding. It stops at `10D2:13C6` after pushing the interrupt
frame; CPU14 and conventional RAM match the modeled pushes. The DOS restore
and subsequent port operations are unexecuted. This path depends on abstract
port callbacks and synthetic time and is not native post-port or PIT parity.

A separate conditional branch supplies the `13B1` model's declared
vector-table-only environment. It restores the saved value `F000:FEA5` to the
caller-owned IVT entry and supplies flags `7246` explicitly; the model stops
at the next unbound port output, `13CC`. The replay checks CPU14 and
conventional RAM against the declared table write and modeled frame. An
unbound DOS service still refuses at 13C6. This is an abstract service: no
same-run native restore, DOS stack/PSP effects, PIT state or full-Up parity is
claimed. The saved vector originates from the earlier conditional install
replay, not an independent ownership determination. The declared environment
contract is documented in [the 13B1 model](RES_08AE_VECTOR_13B1_CPU.md).

Another conditional branch reruns the C-produced 13B1 entry with the declared
vector-only DOS binding, return flags `7246`, an explicit `IN 61h` value `03`,
and an ordered test-only output log: `40:FF`, `40:FF`, `61:00`, then
`C0:9F`, `C0:BF`, `C0:DF`, `C0:FF`. With these declared inputs, the existing
13B1 and dispatcher models return to the coordinate writer at `2E60:0747`;
the test checks CPU14 and all conventional RAM against modeled effects. A
missing `IN 61h` binding refuses at 13D2 after the first two logged writes.
The writer instruction at 0747 is not executed. No native post-port/return,
DOS stack/PSP or device-state parity is implied by these abstract inputs.

From the conditional `0747` return, the test projects the current party
fields and direction table into the existing
[coordinate-writer model](U68_COORDINATE_WRITER_073B.md). Its resident hook
only acknowledges the already-produced conditional return; an absent hook
still refuses before movement. With the lookup buffer deliberately withheld,
the high-level writer reports its lookup stop after projecting x `15 -> 14`
and y `4 -> 4`. The test copies only those two projected fields to the RAM
image and checks that projection against its expected full image. The raw CPU
remains at 0747: this adapter does not model the intervening raw CPU or lookup
call frame, and the projected stop is not a native 07A1 capture. Synthetic time,
abstract device callbacks, native post-port parity and full-Up limits remain.

A separate field-only binding rereads the C-produced DS coordinates, facing,
map byte and far buffer pointer after the coordinate projection. It invokes the
existing [entry-2 nibble model](U3BD_NIBBLE_ENTRY2.md) with an in-range buffer
and projects its result into `DS:6AB0`. On this conditional state it reads
`3DA1:014E = FE`, returning low nibble `0E`; projected x/y remain `(14,4)`.
Missing resident acknowledgement, missing RAM, short memory and an out-of-range
buffer refuse before the projected store. The replay checks the projected RAM
image, but raw CPU remains at 0747: no 0795/07A1 push or raw lookup frame is
synthesized. Synthetic time and abstract device assumptions remain, and no
native post-port or full-Up parity is claimed.

A further high-level writer projection re-evaluates the conditional movement
fields with the C-produced buffer supplied and the window deliberately
withheld. It projects the writer's three-byte fill `DS:6DDE..6DE0 = 01 01 01`
and entry-4 result `DS:6AB1 = 00` from `3DA1:024E = 00`, then reports the
unbound window read at 07CE. The test copies those fields into the projected
RAM image and compares it; in this retained state they already have those
values. The fill behavior follows the existing writer model's argument-shape
inference, not a native fill capture. Raw CPU remains at 0747, no lookup or
window call frame is synthesized, and native post-port/full-Up parity is not
established.

The field-only writer projection can also reread the C-produced window-1 far
pointer and its word at `+0594`. On this conditional state it reads
`3A16:0594 = 0000`, selects arguments `(1,1)`, and refuses the unbound unit
7D hook at the high-level report offset 07ED. The projected RAM remains
unchanged and raw CPU remains at 0747; no raw 07ED call frame is synthesized.
The existing [7D entry model](U7D_ENTRY1.md) needs window-0 words and limit
table, linked-list effects when reached, and the near `0000` callee's effects.
Its update hook is optional, so an adapter must not treat an absent hook as a
completed callee. No 7D effect or native post-port/full-Up parity is claimed.

At that unbound hook, the conditional C-produced image contains window 0 at
`3996:0000` with words `(0,0,5,10,0,0,0)` and limit words
`(10,10,6,24,30,12,256)`. For the projected `(n,idx)=(1,1)`, incrementing
word 1 would leave all seven values below their limits, so this particular
input would not reach the list walk; the actual list pointer is non-null
(`449A:0008`) and is not traversed. The test only inventories these inputs and
leaves RAM unchanged. The 7D call and its near `0000` callee remain unbound;
no optional hook is treated as a completed effect. Raw CPU remains at 0747,
and native post-port and full-Up parity remain unverified.

A separate copy of that conditional image now previews the entry's seven-word
writeback using the existing [7D carry normalizer](U7D_ENTRY1.md). The adapter
requires the `(1,1)` unbound writer report and rejects any input that would
carry before modifying memory. This guard makes the non-null native list
irrelevant to this path; the normalizer receives no host list and reports no
carries or walks. The preview changes only window word 1, from `0000` to
`0001`, across the full RAM image. A forced-carry input refuses with no writes.
The original projected image and unbound writer stop are preserved. This is a
field-only preview up to the call of near `0000`: the optional update hook is
never invoked, its effects remain required, and no return from it is claimed.
Raw CPU still remains at 0747. The synthetic time and device assumptions above
continue to apply; this is neither native 7D execution nor full-Up parity.

A second, separate field-only preview examines the C-produced input to near
`7D:0000` using the existing [bounded near-call model](U7D_ZERO_CALLEE.md).
`DS:49F3` is `4`, selecting the inferred 72-byte fill of flags starting at
`DS:471E`; `(n,idx)=(1,1)` selects one chunk. The far list contains six
members and eight reachable items, all with zero quantity. The preview checks
bounded, nonoverlapping far-pointer ranges and rejects offset wrap or any nonzero quantity before
writing. On that guarded input the model makes no unit `9E` call: it projects
the fill followed by clearing the first six member flags. The test compares
all RAM against that field-only result and verifies a nonzero-quantity case
refuses unchanged. The fill is inferred from its argument shape, and this
host-list model does not represent the raw CPU or call frames. The real writer
hook and near call remain unbound, raw CPU remains at 0747, and no native
post-port or full-Up parity is claimed.

A fixed-input raw prefix now starts from the conditional C-produced CPU at
`2E60:0747` and stops before the entry-2 `LCALL` at `07A1`. The script pins
the loaded `0747..07A0` prefix SHA-256
`2cd34a43bee8924b156e96d13e0ff84e0536ecf447feeba324ee14648514f3c8`;
those 90 bytes also match the corresponding prefix of the hash-pinned writer
in [the coordinate-writer audit](U68_COORDINATE_WRITER_073B.md). The nearby
far-call segment words are relocated, so no file-image identity is assumed
for the whole overlay. The adapter checks the entire frozen CPU profile,
facing `6`, position `(15,4)`, direction bytes `(FF,00)`, the loaded prefix,
1 MiB allocation and conventional-memory backing before any write. It refuses
on missing backing or changed prefix. No access above the 655360-byte captured
range is made; the rest of the allocation is zero-filled and remains unbacked,
not a claim about native high memory.

At the stop the modeled x/y are `(14,4)`; AX and DI are `0006`, SP is
`3DB8`, FLAGS `7297`, and SS stack words from SP are `(0006,0004,000E)`.
Other CPU words are unchanged. The exact replay compares all CPU words and the
full allocated buffer; six bytes differ from its input. The author reports a separate local Unicorn 2.1.4 comparison of all 14 CPU
words and the allocated 1 MiB buffer; its temporary procedure and result are
not retained in this change, so that report is not independently verified. This is a fixed-branch adapter,
not general writer coverage: no call, overlay dispatch, or native post-port
execution is asserted. Synthetic time and device assumptions remain; no
same-invocation or full-Up parity is claimed.

From that fixed prefix, a scratch-only adapter validates the captured `07A1`
far call and `0BF9:002A` stub, which jumps to loaded `2D14:047B`. The
hash-pinned loaded entry-2 body and `031F` leaf match the spans documented in
[the nibble audit](U3BD_NIBBLE_ENTRY2.md). It builds the far-call frame and
runs the existing full-state lookup model, with its accessed stack, code, DS,
and `3DA1:014E` buffer within captured conventional RAM. The model returns
`AL=0E` at `2E60:07A6`; the adapter applies the validated following store to
`DS:6AB0` and stops at `07A9`, before the runtime fill call. The returned CPU
has AX `000E`, BX `0000`, CX `0004`, DX `000E`, SI `0000`, DI `004E`, BP
and SP `3DBE`, CS:IP `2E60:07A9`, DS `14A0`, ES `3DA1`, SS `1D16`, FLAGS
`7202`. The test compares all 14 words and the full allocated RAM image;
18 stack bytes differ across the lookup before the store. Missing backing or
a mismatched stub refuses unchanged. Upper RAM remains unbacked, and earlier
synthetic time/device assumptions remain. This does not establish native
post-port, same-invocation, or full-Up parity.

A further fixed-profile adapter validates the loaded caller at `07A9..07B9`
and its relocated `131C:16A8` target. The loaded 20-byte helper matches the
FillChar hash documented in [the runtime-leaf audit](SYSTEM_RUNTIME_LEAVES.md).
It models the caller's argument pushes and far-call frame, then invokes the
existing raw FillChar model on a scratch image. The destination and stack are
inside captured conventional RAM. The returned state is `2E60:07BA`, AX `0001`,
BX `3DB2`, CX `0000`, DX `000E`, SI `0000`, DI `6DE1`, BP and SP `3DBE`,
DS and ES `14A0`, SS `1D16`, FLAGS `7202`. The fill writes `01 01 01` at
`DS:6DDE..6DE0`, which already hold those values; equality alone does not
prove writes. The 11 changed stack bytes are model-pinned.
The replay compares all 14 CPU words and the full allocated image and verifies
missing backing and changed helper identity refuse without effects. The next
entry-4 lookup has not executed. Upper RAM remains unbacked; earlier synthetic
time and device assumptions remain, and no native post-port, same-invocation,
or full-Up parity is claimed.

The next fixed-profile adapter validates the caller at `07BA..07C9` and the
loaded `0BF9:0034` stub to `2D14:057A`. The loaded entry-4 body matches the
hash documented in [the cell lookup audit](U3BD_CELL_ENTRY4.md); the shared
`031F` leaf is pinned above. It builds the far-call frame and runs the existing
raw entry-4 model with the reached buffer in captured conventional memory,
then applies the validated result store and stops at `2E60:07CA`. The model
returns `AL=00`, stored at `DS:6AB1`, which was already zero; equality alone
does not demonstrate a value-changing write. The replay compares the full
allocated RAM and CPU14, with 21 model-pinned stack bytes differing across the
call. Missing backing and a changed stub refuse unchanged. Upper RAM remains
unbacked, earlier time/device inputs are synthetic, and native post-port,
same-invocation, and full-Up parity are not claimed.

A final fixed-profile writer prefix validates the loaded `07CA..07EC` bytes,
CPU and conventional backing, then rereads window pointer `3A16:0000` and word
`+0594 = 0000`. The bit-clear branch pushes `(1,1)` and stops before the
unbound call at `2E60:07ED`. The bit-clear path loads AL with `01` before each
`PUSH AX`, so AX at the boundary is `0001`. Its model-pinned state has AX
`0001`, BX `3DB2`, CX `0004`, DX `000E`, SI and DI `0000`, BP `3DBE`, SP
`3DBA`, DS `14A0`,
ES `3A16`, SS `1D16`, FLAGS `7246`; the two argument words are `0001,0001`.
Two stack bytes differ from the entry-4 return. The replay compares all CPU
words and the allocated RAM buffer; missing backing, changed code, or a
changed window value refuse without effects. The 7D call and overlay transition
remain unbound; no unit 7D call frame is synthesized. Upper RAM remains
unbacked and earlier time and device inputs remain synthetic; no native
post-port, same-invocation, or full-Up parity is claimed.

The retained `07ED` far call targets `08B0:0025`. Its hash-pinned stub begins
with `INT 3Fh`, the cold overlay-manager form described in
[the overlay preflight boundary](OVL_0068_068D_CPU.md), rather than a loaded
far jump. A fixed-profile adapter validates the call and stub bytes, backing,
CPU and `(1,1)` argument words, pushes the far return `2E60:07F2`, and stops
at `08B0:0025` before the interrupt. The resulting SP is `3DB6`, FLAGS remain
`7246`, and all other registers are unchanged except CS:IP; one stack byte
differs. The pinned replay checks all CPU words and allocated RAM and refuses
changed stub identity or missing backing without effects. The overlay manager,
unit 7D entry, and near `0000` call remain unexecuted. A separate native watch's
loaded entry frame does not establish this conditional loader transition. Upper
RAM is unbacked, earlier time/device inputs are synthetic, and no native
post-port, same-invocation, or full-Up parity is claimed.

The conditional image's INT 3Fh vector is `12C6:02E6`; the loaded 82-byte
handler matches the hash in [the loaded-handler evidence](U9E_ENTRY8_LOADED_HANDLER_WINDOW.md).
A fixed-profile adapter validates the cold stub, vector, handler, CPU, far-call
frame and backing, then models only interrupt entry: it pushes FLAGS `7246`,
CS `08B0`, and return IP `0027`, clears IF and TF, and stops at `12C6:02E6`
with SP `3DB0` and FLAGS `7046`. The replay checks all CPU words and allocated
RAM; five stack bytes differ. Missing backing, changed vector or changed handler
refuse before effects. This executes no handler instruction and does not establish
overlay loading, a unit 7D entry frame, or near `0000` effects. Upper RAM is
unbacked, earlier time/device inputs are synthetic, and native post-port,
same-invocation, and full-Up parity remain unverified.
