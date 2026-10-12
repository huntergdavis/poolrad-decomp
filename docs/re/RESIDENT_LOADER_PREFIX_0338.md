# Resident cold-loader prefix at `+0338`

The loaded resident routine at `12C6:0338` now has a bounded C model
through its **first reached nested CALL**. On the pinned saved slot-A
route, execution stops before `+0352 CALL +0553`; with nonzero
`ES:[0010]`, it stops before `+03B6 CALL +0435`. The model reproduces
all reached CPU and RAM effects before either call. It does not model
either callee, the loader tail, the eventual overlay load, the handler
return, or unit 0x9B. It adds no full-routine or identity count.

Before implementation, `deja "Poolrad cold loader 12C6 0338 03E9
0553 03AA run038 native capture"` timed out during index refresh
(exit 124), so it yielded no session result. The merged
[first-insertion native CPU note](SLOT_A_NATIVE_FIRST_INSERT_COLD_CPU.md)
and [handler-prefix note](RESIDENT_HANDLER_PREFIX_02E6.md) supplied the
loaded segment and candidate boundary. This work independently
rechecked original and loaded bytes, instruction paths, and a fresh
same-run native entry/exit pair.

## Byte and branch boundary

| Evidence | Result |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded original `0AA2:0338..03E8` | 177 bytes; 64 16-bit instructions, 12 branches, 12 calls; SHA-256 `58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991` |
| Loaded `12C6:0338..03E8` | Same 177-byte SHA-256; zero differing bytes in the fresh entry RAM |

The path begins at `+0338` by incrementing `DS:[43E0]`, then compares
`ES:[0010]` with zero at `+033C`. The saved zero value takes `+0342 JE
+034D`, increments `DS:[43E2]`, pushes ES at `+0351`, and reaches the
unexecuted `+0352 CALL +0553`. The nonzero route writes one to
`ES:[0012]` at `+0344`, jumps at `+034B`, and reaches the unexecuted
`+03B6 CALL +0435`. Both routes preserve the side effects and CPU flags
of the instructions already executed. The native saved-route capture
supports the zero path; the nonzero path is instruction-derived and
synthetic-tested.

## Fresh saved-route native parity

The private run `slot-a-loader-firstcall-002` used the ignored slot-A
save with SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
It directly stopped at loaded `12C6:0338` and then at `12C6:0352`,
before the nested call executed. Its 207-byte numeric CPU transcript
has SHA-256
`034a1539e1ab2a00d273ff32aa03adf517cf98fdfd36b743864f209352a4c6a1`.
Both snapshots are 655,360 bytes. Entry SHA-256 is
`9d40ff74d9deb791bbfc9527474cd29aff7ba3fd215cd9c3eaedcdef21c06614`;
pre-call SHA-256 is
`3861865f8457dc946ab8760847a53c8ad78e47a7c9bcafb1c43e30a48d5b3bac`.
The entry CPU is `CS:IP=12C6:0338`, `DS=14A0`, `ES=08D0`,
`SS:SP=1D16:377A`, `BP=3794`, and `FLAGS=7212`. The pre-call stop is
`12C6:0352`, `SP=3778`, `FLAGS=7202`; the other 11 modeled CPU fields
are unchanged. The word at `DS:[43E0]` and `DS:[43E2]` changes from
`0029` to `002A`; the pushed ES word occupies `SS:[3778]`.

Compiled C reproduces every byte of the captured 640 KiB native
pre-call RAM and all 14 CPU fields from the same-run entry. Exactly
four RAM bytes differ between the entry and stop, in three contiguous
address runs. The private RAM capture does not include the upper
384 KiB of the C model's 1 MiB RAM space, which the audit initializes
to zero; it makes no native parity claim for that region. A previous
run-038 loader entry has the same CPU tuple but a different whole-RAM
hash, so the parity comparison uses only the fresh same-run pair.

Focused synthetic tests cover the saved zero route, nonzero route,
second increment overflow/flags, segment-offset stack-word crossing,
and invalid arguments without mutation. The crossing test asserts
that an offset `FFFF` word uses offset `0000` in the same segment.
No native nonzero-route stop was captured. The portable API returns
the next reached call boundary and leaves execution of that call to
the host; full cold-loader parity remains open.

Reproduce locally with ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_prefix_0338_audit.py
cmake -S . -B /tmp/poolrad-loader-prefix-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-prefix-build
ctest --test-dir /tmp/poolrad-loader-prefix-build --output-on-failure
```

The audit prints only hashes, addresses, and counts. Original code,
RAM captures, saves, and media remain in ignored `resources/`.
