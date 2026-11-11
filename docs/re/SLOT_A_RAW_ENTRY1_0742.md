# Slot A run002: raw bounded entry-1 handoff to 0742

The Linux replay `check_slot_a_raw_entry1_0742.sh` verifies the hashes of
same-invocation segmented `0D6D` and `0742` conventional RAM captures and the
manual CPU14 transcriptions. A narrow C adapter executes the captured path
from the former entry, invokes the existing full-state `3BD:0350` lookup
through its checked loaded far stub, and refuses unsupported branches. It
stops before the resident far call at `0742`; it does not perform that call.
The adapter maps the captured 640 KiB and protects the uncaptured upper
384 KiB, so an upper-memory access faults rather than reading invented bytes.

```sh
bash tests/replay/check_slot_a_raw_entry1_0742.sh LIBPOOLRAD_RECOVERED_A PRIVATE_RUN002_DIR
```

For the pinned run, the derived 14 CPU fields match the `0742` transcription,
and all 655,360 conventional RAM bytes match the independently captured
same-run `0742` image. The code and relocation data are pinned through the
input image hash; the adapter is deliberately restricted to the observed
selector, lookup result and loaded stub. This is a bounded test adapter, not
a general implementation or new function credit. The CPU records are manual
debugger transcriptions; the execution assumes no unobserved interrupts and
does not establish device or upper-memory parity. The upstream `348D`
single-dump image for this run is invalid, so this does not establish
continuous execution from `348D` or a `3492` result.

The next boundary is the resident call. The separately validated
[resident prefix](SLOT_A_C_RESIDENT_PREFIX_1386.md) starts from the captured
`0742` state and stops at the first DOS effect; wiring a continuous handoff
requires carrying this C-produced state rather than loading that checkpoint.
Same-invocation DOS service, PIT/interrupt/device and return effects remain
unavailable; no complete post-Up or native parity claim follows.
