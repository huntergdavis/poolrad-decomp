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
