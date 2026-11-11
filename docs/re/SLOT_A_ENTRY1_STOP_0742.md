# Slot A entry 1: bounded stop at the coordinate writer's resident call

Using the same native invocation described in the private run002 capture,
the harness reads its hash-locked 640 KiB segmented `0D6D` entry image,
resolves its window and field pointers, and drives `pr_u68_entry1_fallible_step`
into the existing coordinate writer. The writer's resident hook explicitly
refuses. The C model reports entry-3 result 1, a committed move call, and a
`+0742` stop with resident word 10. Its selected party `(15,4,6)`, three
`6DDE` bytes, and tail bytes `6B00/6B05` match the independently captured
same-run `0742` image; the resident argument matches the native stack word.
No later entry-1 hooks or tail writes run.

To reproduce with locally held ignored captures and a built recovered library:

```sh
bash tests/replay/check_slot_a_entry1_stop_0742.sh LIBPOOLRAD_RECOVERED_A RAM_0D6D_SEGMENTED RAM_0742_SEGMENTED
```

The script verifies the two image hashes; the original segmented manifests
and debugger records are described in the private capture review. The older
single-dump `348D` and `0D6D` files are invalid repeated-segment dumps and are
not used. This is a selected-data and stop comparison, not CPU, full RAM,
interrupt, device, or native parity. The C report does not reproduce the
native stack frame. The next effect is resident `08AE:00DC`, whose reached
DOS vector return and RAM effects, PIT/device effects, and actual return are
not available as a complete same-invocation sequence; no effect is supplied
by this harness. It does not reach `3492` or establish a post-Up C frame.

Prior evidence reused: [direct post-EXIT return](SLOT_A_POST_EXIT_NATIVE_RETURN.md),
[entry-1 model](U68_ENTRY1.md), and the private `t4492-capture-review.md`.
