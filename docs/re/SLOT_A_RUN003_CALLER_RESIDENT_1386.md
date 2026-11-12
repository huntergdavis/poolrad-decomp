# Run003 caller to resident first DOS boundary

This bounded captured-path replay starts from the private run003 `3582:348D`
CPU and conventional RAM capture, immediately before the unit-68 entry-1 far
call. It checks the observed call to `0896:0025`, the live far-jump stub to
`2E60:0D6D`, then composes the existing entry-1 adapter, lookup, resident
`00DC` dispatch, and `137C` vector model. The first DOS callback refuses after
the model pushes the interrupt frame. No intermediate native RAM or CPU is
loaded as execution input.

```sh
bash tests/replay/check_slot_a_run003_caller_resident_1386.sh LIBPOOLRAD_RECOVERED_A PRIVATE_RUN003_DIR
```

The script locks the same-run 348D and 1386 RAM and CPU capture hashes. The
manually transcribed 1386 pre-INT CPU matches all 14 fields and the reconstructed
pre-frame C memory matches all 655,360 conventional RAM bytes. The six bytes
overwritten by the refused INT frame are saved from the C-produced state before
the vector model runs. The uncaptured upper 384 KiB is protected against access.
This is one observed path, assumes no unobserved interrupts, and does not prove
DOS, PIT, 3492, or a complete Up frame. At this stop the DOS service is refused.

Reuse: [run002 continuous prefix](SLOT_A_CONTINUOUS_C_1386.md) provides the
entry and resident composition; [direct caller trace](SLOT_A_POST_EXIT_NATIVE_RETURN.md)
identifies 348D. This replay substitutes a hash-locked same-run 348D input and
executes the call and already-resolved jump without importing the run002 state.
