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
has not been established. The second DOS interrupt at 139E and subsequent effects
are outside this test. The normal 137C entry and its default refusal remain
unchanged.

The replay then continues that C-produced 1388 state, without reloading RAM or
CPU, through the resident vector-save and setup instructions to the second DOS
interrupt at 139E. It matches the same run003 139E capture in all 14 CPU fields
and all 655,360 conventional RAM bytes. The second interrupt is not executed;
its handler and subsequent effects remain unresolved in this composition.
This extends the bounded service replay, not the earlier run002-seeded post-Up
prefix. A prior independent full-image 1388-to-139E observation appears in
shared memory session `01a13101-709d-77e1-9fcb-2a6acbff0046`.
