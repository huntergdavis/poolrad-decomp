# Declared boundary: `RES_0A40_02FD` (BIOS `INT 16h` AH=1, key pending)

**Status: declared boundary contract — USABLE as a dependency, NEVER a credit.**
A row in this class may be composed by a caller and may never appear in
`strict_full_c_addresses.csv` or `reviewed_function_identities.csv`; the queue
loader refuses that and `test_strict_candidate_queue` proves the refusal.

## Extent
`0A40:02FD`, **18 bytes**, ends `retf` (no argument). Loader-expanded image, SHA-256
`83af1ee6…`.

## What it is
The complete body:

```
02FD  cmp byte ptr [8501], 0      ; buffered extended-key byte
0302  jne 030C                    ; pending buffered key -> return 1
0304  mov ah, 1
0306  int 16h                     ; BIOS: keyboard status
0308  mov al, 0
030A  je 030E                     ; ZF set (no key) -> return 0
030C  mov al, 1
030E  retf
```

It is a **thin BIOS query**: report whether a key is available, preferring the
program's own buffered extended-key byte at `DS:0x8501`.

## Modelled
- The register contract: `AL` = 0 or 1, and the flags the caller may observe.
- The `DS:0x8501` buffered-key preference and the branch it controls.
- Frame and return: no stack effect beyond `retf`.

## NOT modelled
- The BIOS keyboard service itself: no 8042 keyboard controller, no scan-code
  translation, no keyboard buffer semantics beyond the observed `AH=1` result.
- Interrupt latency, `INT 16h` hooking by other software, and hardware state.
- Where the byte at `DS:0x8501` comes from — that writer is a separate body.

## How behaviour is supplied
The **differential**, not this document: the original 18 bytes execute under
Unicorn (and DOSBox-X in the native captures) against the same RAM image, and the
caller's C model is compared to that execution over all 14 CPU words and the full
1 MiB plus the interface's return value. The declaration states the contract; the
harness measures it.

## Limits
- Composable does not mean credited: this endpoint has no C translation and no
  identity credit, and composing it does not imply either.
- A caller that composes this endpoint inherits a **declared boundary**, which is
  recorded in that caller's own evidence document.
- If a future session wants to translate it, the translation must reproduce the
  BIOS call's contract as measured, not as described here.
