# Declared boundary: `RES_0A40_030F` (BIOS `INT 16h` AH=0, read key + CRT buffer)

**Status: declared boundary contract — USABLE as a dependency, NEVER a credit.**
Never appears in `strict_full_c_addresses.csv` or `reviewed_function_identities.csv`;
the queue loader refuses that and `test_strict_candidate_queue` proves it.

## Extent
`0A40:030F`, **34 bytes**, `RETF` (no argument). Loader-expanded image, SHA-256
`10e41540…`.

## What it is
```
030F  mov al, [8501]     ; consume the buffered extended-key byte
0312  mov byte [8501], 0
0317  or al, al
0319  jne 032D           ; buffered key present -> use it
031B  xor ah, ah
031D  int 16h            ; BIOS: read key (blocking)
031F  or al, al
0321  jne 032D           ; normal ASCII key
0323  mov [8501], ah     ; extended key: buffer the scan code
0327  or ah, ah
0329  jne 032D
032B  mov al, 3          ; neither -> synthesise Ctrl-C
032D  call 0143          ; CRT helper inside the same segment
0330  retf
```

## Modelled
- `AL`/`AH` contract: returns the ASCII byte (or the synthesised `3`), buffering
  the extended-key scan code at `DS:0x8501` for the next query.
- The `DS:0x8501` buffer protocol, shared with `RES_0A40_02FD`.
- The `call 0143` CRT helper belongs to the same declared boundary.

## NOT modelled
- The BIOS read-key service, the keyboard controller, scan-code sets, and any
  TSR that hooks `INT 16h`.
- Blocking semantics: in the harness the key source is the measured execution,
  not a live keyboard.
- Whether `0143` has side effects outside this segment — not investigated here.

## How behaviour is supplied
The differential: the original bytes execute under Unicorn/DOSBox-X against the
same RAM image, and the caller's C model is compared over all 14 CPU words and the
full 1 MiB. The declaration states the contract; the harness measures it.

## Limits
- Composable, not credited.
- A composing caller records the boundary in its own evidence document.
- The synthesised `AL=3` path is part of the observed contract and must not be
  "tidied up" by a future translation.
