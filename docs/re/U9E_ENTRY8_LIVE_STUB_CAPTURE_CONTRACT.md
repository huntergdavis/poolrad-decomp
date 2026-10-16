# U9E entry8 relocated stub observation and live seam contract

This local closure prerequisite follows the merged
[U7F-to-U9E source dispatch boundary](U9E_ENTRY8_U7F_DISPATCH_BOUNDARY.md)
and [loaded-entry U9E composition](U9E_ENTRY8_FULLSTATE_COMPOSITION_0DE3.md).
The targeted `deja` query stalled and was stopped without a recall result.
The merged [resident loader map](RESIDENT_LOADER_MAP.md),
[INT 3Fh handler prefix](RESIDENT_HANDLER_PREFIX_02E6.md), and
[U9B reached dispatch](U9B_RUNTIME_DISPATCH_0000.md) provide separate
loader precedent. None is a reached U7F-to-U9E dispatch frame.

The source-image U7F +0FDE far call is `9A 48 00 B0 00` at GAME file
`0x25C67`, with SHA-256
`e4551a2bc99d5c8ac9cc97784911c182c2470f56668e8cf8bdb3a895ea4d9115`.
The expanded resident-image U9E entry-8 stub is `CD 3F E3 0D 00` at
image `0xB48` (`B0:0048` **before** DOS load relocation), SHA-256
`16e87d451726931b6e084c2a73e82f385b41e3886c5d7a9806ec0fd814f991f2`.
The original U9E entry8 body remains GAME file `[0x2BCCD,0x2BD18)`,
75 bytes / 29 starts, SHA-256
`7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba`.
The original resident INT 3Fh handler `0AA2:+02E6..+0337` is 82 bytes,
SHA-256 `6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c`.

## Retained Up-run RAM, not a U7F call

The private slot-A one-Up watch has 640 KiB RAM dumps at U1C +3395,
+33BD, and +026C with SHA-256 respectively
`4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc`,
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`,
and `d5332052a2367728a88215e36ff182a4f4e7a2bbdde22a7ef2c74497e2f0cf7c`.
The new `check_overlay_u9e_entry8_up_stub_state.py` rehashes these
private dumps and the original media, then checks only their measured
addresses:

| Stop | U9E stub at loaded `08D4:0048` | IVT 3Fh | Positive-control U68 `0896:002A` |
| --- | --- | --- | --- |
| U1C +3395 before Up | `CD 3F E3 0D 00` | `12C6:02E6` | cold `CD 3F 89 09 00` |
| U1C +33BD after Up | same cold U9E bytes | same | patched `EA 89 09 60 2E` |
| U1C +026C before RANDOM | same cold U9E bytes | same | same patched U68 bytes |

The `+0x824` paragraph difference maps source U9E `B0` to observed
`08D4` and source U68 `72` to observed `0896` **in this run**.
Unrelocated `00B0:0048` is five zero bytes in each retained RAM dump;
it is not the actual loaded stub address. The U68 change demonstrates
that another reached stub was patched between these stops. It does
not prove any U9E dispatch, loaded U9E code segment, or U7F caller
reach. The 640 KiB dumps lack upper RAM and a complete CPU14 at these
cuts. Do not use `08D4` or `12C6` as fixed addresses for another run.

## Full-state same-run seam, held

When host native execution is released, capture one naturally reached
U7F +0FDE far call. At each cut below, keep the same run ID and event
ordinal, actual CS:IP and all 14 CPU words
`AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS`, a full 1 MiB
A20-off RAM file/hash, loaded instruction bytes/hash, and the action
transcript. The owner of the U7F callback must also preserve 16-bit
SS offset wrap and each word's physically contiguous second byte.

1. At U7F +0FDE before the call, record the relocated far target and
   the two live argument WORDs. At the reached stub, read actual return
   IP/CS, range and count from SS offsets `SP`, `SP+2`, `SP+4`, `SP+6`.
   A normal source return IP is +0FE3, but the live frame decides.
2. Record the *actual* stub bytes. If `CD 3F`, capture vector 3Fh,
   interrupt entry frame, handler +02E6, selected +0338 loader path,
   any DOS/service effects, pre-IRET frame, patched stub and its target.
   If an `EA` far jump was already patched, capture that direct target
   and its preceding state. Any other form or nonreturn is an explicit
   stop, not a forced loaded U9E entry.
3. At a genuinely reached loaded U9E +0DE3, verify its loaded CS and
   original instruction bytes, full CPU14/RAM and retained U7F return
   frame. Capture SYSTEM +11C0/+120F calls and the actual +0E2B RETF4
   or changed transfer, then U7F +0FE3 only if reached.

Only a same-run full-state dispatch comparison can connect the
source-image `B0:0048` callback to
`pr_u9e_entry8_fullstate_compose_0de3`. The merged resident handler
prefix begins **after** the INT frame and stops before the loader;
the U9E composition begins **after** dispatch. Neither supplies the
missing segment mapping or net RAM/CPU14 effects. A changed stub,
return frame, alias, transfer, or unobserved service must remain an
explicit outcome. Endpoint RAM differences do not prove transient
write order or external device state. No native run was started for
this gate; natural reach, full dispatch parity, and strict-C credit
remain open. The project strict count remains 82.

Read-only verification:

```sh
python3 tests/recovered/check_overlay_u9e_entry8_up_stub_state.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --pre-input-ram <ignored-pre-call-3395-ram.bin> \
  --post-up-ram <ignored-loop-gate-33bd-ram.bin> \
  --later-ram <ignored-pre-random-026c-ram.bin>
```
