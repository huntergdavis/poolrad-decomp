# Second-key resident `+006B` return dependencies

This note classifies the reached `RES_08AE:+006B` far return in the
second-key slot-A replay. It does not translate the intervening resident
instructions or compose a complete second-key call. The required
`deja "Poolrad RES_08AE +006B RETF +008D caller return dependencies second key"`
search found no indexed prior session. The merged
[`+002A` caller return gate](SLOT_A_SECOND_KEY_RESIDENT_002A_CALLER_RETURN.md)
and [`+001F..+0033` caller composition](SLOT_A_SECOND_KEY_RESIDENT_001F_0033_COMPOSED.md)
provide the earlier caller boundary; their arithmetic-helper output is a
supplied native state.

## Reached native boundary

The controlled replay used DOS v1.3 `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and the owner slot-A save SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
All captured media and RAM remain ignored under
`resources/oracle-runs/slot-a-resident-137c-second-004/`.

At loaded `10D2:006B`, the instruction byte is `CB` (`RETF`). The CPU words
below are in DOSBox debugger order
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`:

| Stop | CPU words | RAM evidence |
| --- | --- | --- |
| before `RETF`, `10D2:006B` | `10D2 006B 14A0 1D16 1D16 3DA8 3DB4 8D94 0005 8D7C 0005 0001 0000 7206` | `ram-10d2-006b-return-007.bin`, 655,360 bytes, SHA-256 `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| after `RETF`, `10D2:008D` | `10D2 008D 14A0 1D16 1D16 3DAC 3DB4 8D94 0005 8D7C 0005 0001 0000 7206` | direct debugger CPU stop; no separate `+008D` RAM file |
| next call site, `10D2:0089` | reached again after the caller loop | `ram-10d2-0089-call-010.bin`, byte-identical to `+006B` RAM, same SHA-256 |

The four bytes at `SS:3DA8` in both saved RAM files are `8D 00 D2 10`:
little-endian far return `10D2:008D`. The `RETF` advances SP by four and
changes CS:IP to that frame; the other 11 CPU words are identical at the two
direct debugger stops. The equal `+006B` and later `+0089` RAM files also
show no lasting byte-value difference between those checkpoints. They do not
rule out same-value or transient stores, and no direct full-RAM comparison to
`+008D` is claimed.

## Dependency order

| Reached span | Established evidence | Remaining boundary |
| --- | --- | --- |
| resident `+0000` callee entered by `+008A` | loaded `+0089..+008C` is `0E E8 73 FF`, `PUSH CS; CALL +0000`; its return IP is `+008D` | the callee body is only partially modeled |
| `+003F..+006A` inside the resident callee | direct checkpoints reached `+0043` and `+006B` | no end-to-end C model or full CPU/RAM parity for this internal span |
| `+006B` far return | loaded `CB`, saved frame, and direct `+006B/+008D` CPU stops above | an instruction-level C adapter would still need a native-seeded full-RAM stop at `+008D` for strict comparison |
| caller `+008D` through `+00AA` | `+008D` is the return successor; native caller then reaches `+00AA` | local stores and intervening comparisons are not composed into this note |
| caller `+00AA` and `+00C4` | separate bounded local gates `2ca13ac` and `97318ec` model later reached branches | their gate parity does not cover the preceding callee or caller span |

The next nested call site at `+0089` begins another `+0000` callee visit.
Its loaded four-byte sequence `0E E8 73 FF` has SHA-256
`781a0883151a1e086bb5f570ebb84118040b622cdb8b94a3da2a9b5e8f572ab1`.
The repeated RAM hash alone does not make the two visits interchangeable;
their CPU registers and stack frames must be supplied from the reached stop.

## Parity review

- **target_function:** the one-byte `RES_08AE:+006B` return and its
  dependency boundaries, not the whole `+0000` callee.
- **source_candidates:** an instruction-level `RETF` adapter is a future
  candidate; no C implementation is added here.
- **signals:** loaded `CB`, exact saved far return frame, direct CPU stops
  with the expected CS:IP and SP transition, and equal later RAM images.
- **score and verdict:** 3/4 direct signal groups support the instruction's
  return semantics (**likely_match**); full callee parity remains **unclear**.
- **review_notes:** no independent second replay and no `+008D` RAM capture
  exist in this note. The large `+003F..+006A` and `+008D..+00AA` spans are
  unmodeled. This classification earns zero strict full-function or game
  replay credit.
