# Slot A run002: continuous bounded C prefix to DOS refusal

This focused replay composes the pinned `0D6D` entry adapter and existing
full-state lookup with the resident `00DC` dispatch and `137C` vector models.
The resident call consumes the CPU and RAM produced by the preceding C
execution in the same mapping; the native `0742` image is used only for a
comparison and is never loaded as the next execution state. The uncaptured
upper 384 KiB remains `PROT_NONE`. The DOS callback refuses at the first
interrupt, after the C model has pushed its hardware frame and cleared IF/TF.

```sh
bash tests/replay/check_slot_a_continuous_resident_1386.sh LIBPOOLRAD_RECOVERED_A PRIVATE_RUN002_DIR
```

For the hash-locked same-run captures, C-derived `0742` CPU14 matches the
manual transcription and all 655,360 conventional RAM bytes match. After
continuing from that C state, the reconstructed pre-INT `1386` CPU14 and
all 655,360 conventional RAM bytes match the native transcription and image.
For this comparison, the six bytes later overwritten by the model's interrupt
frame are read from the C-produced pre-frame state, saved before the vector
model runs. Its preceding DS/ES pushes do not overlap those six bytes. No
native checkpoint is substituted for the C-produced state.

This is a bounded captured-path test adapter, not a general entry-1
implementation. The manual CPU transcriptions and assumption of no
unobserved interrupts limit the evidence. It starts at valid `0D6D`, not
`348D` (the run002 single `348D` dump is invalid), and it does not establish
upper-memory or device parity, DOS results, `3492` or a complete Up frame.
The next missing effect is the same-invocation DOS vector service return,
flags and all RAM effects; PIT, interrupts, device state and actual return
remain unmodeled. See [raw entry handoff](SLOT_A_RAW_ENTRY1_0742.md) and
[C resident prefix](SLOT_A_C_RESIDENT_PREFIX_1386.md).
