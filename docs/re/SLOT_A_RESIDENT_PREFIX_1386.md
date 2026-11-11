# Slot A run002: checked resident prefix to pre-INT

The private run002 segmented `0742` and `1386` captures and their manually
transcribed CPU14 files are pinned by hash in
`tests/replay/check_slot_a_resident_prefix.py`. The script starts at the
captured `2E60:0742` far call, checks its loaded target and resident code
identities, and models only the reached path through resident `00DC` and
`137C`. It checks every read and stack write against the captured 640 KiB,
and refuses out-of-range access. It stops at `10D2:1386` **before** INT 21h;
no DOS callback, interrupt frame, port, or device effect is supplied.

```sh
python3 tests/replay/check_slot_a_resident_prefix.py PRIVATE_RUN002_DIR
```

For the pinned capture, the derived CPU14 values and 14 bytes written by the
modeled stack operations match the same-run pre-INT transcription and RAM.
The pre-call instruction implies the far-call frame; the intermediate `00DC`
entry was not separately observed. This experiment assumes no unobserved
interrupt between the stops. CPU files are manual debugger transcriptions,
not independent machine-register audits. Other RAM, devices, and full native
execution are not compared. The script is a narrow independent model; it does
not call the existing 1 MiB raw resident driver or alter the entry-1 `0742`
refusal. It does not establish `3492` or post-Up C parity.

The next unavailable effect is the actual same-invocation DOS vector service
return and all of its RAM/flag effects. A callback supplied to the existing
`137C` model sees state after the INT hardware frame and IF/TF clearing; that
is not the pre-INT state compared here. See the private
`t4492-service-review.md`, [the entry stop](SLOT_A_ENTRY1_STOP_0742.md), and
[the resident driver contract](RES_08AE_VECTOR_137C_CPU.md).
