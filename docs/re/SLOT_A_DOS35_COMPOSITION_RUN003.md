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
