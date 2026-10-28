# Second-key resident caller prefix `RES_08AE:+001F..+0029`

A controlled key-route debugger capture reached loaded resident
`10D2:+001F` after a DOS-time wrapper return, then ran to `+002A`
before the far call. It came from a **later iteration of the same
controlled process** than an earlier local second-call `+001F`
capture: the resumed debugger was at another `1256:00A7` DOS-time
call. The two RAM images must not be paired across iterations.

## Source and bound

The required `deja` query for this caller span could not open its
read-only index lock in this sandbox. An earlier read-only recall for
the same span found no exact prior session. The unpublished local
`SLOT_A_SECOND_KEY_TIME_00A2.md` note suggested the first-iteration
address; the bytes and CPU/RAM claims below were checked again against
this fresh later-iteration capture.

Original `08AE:+001F..+0029` occupies expanded START image
`0x8AFF..0x8B09` and 11 contiguous START file bytes at
`0x8B16..0x8B20`. Input START SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Original bytes and code bytes read directly from the fresh `+001F`
RAM image match:
`[scrubbed: span=SLOT_A_SECOND_KEY_RESIDENT_001F_CALLER_PREFIX-1 sha256=5914f5b94141e45e53a07d4410d2a475db1ea0486bf0f7f07717114e85d77ad8 bytes=10 offset=UNRESOLVED]00`, SHA-256
`62cb8d36ef61a54aaf646a25e4873eb1013966a703f3992f1428d793f1502a94`.
They decode to `MOV AX,SS:[BP-6]`, `XOR DX,DX`, `MOV CX,7E40h`,
and `MOV BX,0005h`. At the stop, the **unexecuted** next instruction
is a far call at `+002A` to the arithmetic helper. Its original
segment is `0AF8`; the loaded segment is `131C`. The arithmetic
helper body and its side effects are outside this gate.

## Fresh same-run stops

The 14 CPU words are ordered
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`.

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| caller return `10D2:001F` | `10D2 001F 14A0 1D16 1D16 3D9A 3DA6 0001 0000 0100 2718 0001 3DA2 7246` | `770d855ea3c015ac208df429bf5ea6eeadffa87c346f1672aa885a6f65f99cfe` |
| before far call `10D2:002A` | `10D2 002A 14A0 1D16 1D16 3D9A 3DA6 0001 0005 7E40 0000 0001 3DA2 7246` | same |

The `1256:00A9` post-DOS stop immediately before the return had
`AX=2C00`, `BX=0000`, `CX=0100`, `DX=2718`, `BP=SP=3D84`,
and flags `7216`. At `+001F`, SS:`3DA0..3DA1` (`[BP-6]`) held
`01 00`. The `+001F..+002A` run changed **zero** of 655,360 RAM
bytes. AX loaded `0001`, DX became zero, CX became `7E40`, and BX
became `0005`; the observed flags stayed `7246`. No far-call stack
write or arithmetic helper instruction executed in this capture.

## Bounded C parity

`pr_rec_res_08ae_001f_prepare_multiply` reads the caller's local
WORD, models the zero-result XOR flags and two immediate MOVs, and
stops with IP=`002A`. A synthetic test checks a nonzero local WORD,
nonzero incoming DX, and the flags independently of AX. The ignored
native-seeded harness compares the complete later-iteration entry and
return RAM images and all 14 CPU words: **0/655,360 RAM mismatches and
0/14 CPU-word mismatches** under strict C11 warnings. The registered
focused CTest also passes. Private captures and harness are under
ignored `resources/oracle-runs/slot-a-resident-137c-second-004/`.

This verifies only the reached caller prefix. The next far call,
its arithmetic effects, later resident branch choices, and a direct
same-iteration continuation from the earlier second-call `+001F`
capture remain separate gates.

## Parity review

- **target_function:** `RES_08AE:+001F..+0029`, stopping before `+002A`.
- **source_candidates:** `pr_rec_res_08ae_001f_prepare_multiply` is the
  candidate for this bounded prefix; it makes no claim for `+002A`.
- **signals:** 11 exact original and fresh runtime bytes; four matching
  decoded operations and immediate constants; SS:`[BP-6]` input `0001`;
  zero native RAM changes; 14/14 native CPU words matched by the C
  replay; synthetic nonzero local-word and DX case.
- **score and verdict:** 4/5 corroborating signal groups, **match**
  for this reached later-iteration prefix. The missing fifth signal is
  a direct `+002A` stop paired with the earlier second-call `+001F`.
- **review_notes:** The API relies on its caller mapping `local_word`
  to SS:`[BP-6]`, which the private harness checks. XOR's auxiliary
  flag is architecturally undefined; this model clears it to match the
  observed DOSBox-X CPU. The prior second-call `+001F` RAM image was
  not paired with this later-iteration `+002A` image.
