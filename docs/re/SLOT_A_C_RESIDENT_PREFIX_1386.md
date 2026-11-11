# Slot A run002: C resident prefix to the first DOS refusal

The focused Linux replay uses the existing `pr_r08ae_dispatch_00dc_cpu` and
`pr_r08ae_vector_137c_cpu` models after deriving the far-call frame from the
hash-locked same-run `0742` state. A 1 MiB mapping exposes the captured lower
640 KiB and protects the remaining 384 KiB with `PROT_NONE`; an access to the
uncaptured range faults instead of returning invented data. The earlier
[checked prefix experiment](SLOT_A_RESIDENT_PREFIX_1386.md) validates the
pinned input hashes, loaded code and CPU transcriptions before the C harness.

```sh
bash tests/replay/check_slot_a_c_resident_prefix.sh LIBPOOLRAD_RECOVERED_A PRIVATE_RUN002_DIR
```

On the pinned capture, the dispatch reaches `137C` and the vector model
reaches a refusing DOS callback. That callback sees the hardware interrupt
frame already pushed and IF/TF cleared. The replay reconstructs the pre-INT
CPU from that frame and compares its 14 fields and the 14 modeled prior stack
bytes with the same-run `1386` capture. The callback refuses without
supplying DOS results or writes. This test does not alter the entry-1 `0742`
refusal or compose the two replay harnesses.

The far-call entry is instruction-derived, and the comparison assumes no
unobserved interrupt; CPU files are manual debugger transcriptions. The
protected mapping rules out accesses to the uncaptured high region on this
execution, not arbitrary profiles or device behavior. This is a selected CPU
and stack comparison, not complete RAM/device parity. Same-invocation DOS
service returns and all effects remain unavailable; no `3492` or full
post-Up C parity is claimed.
