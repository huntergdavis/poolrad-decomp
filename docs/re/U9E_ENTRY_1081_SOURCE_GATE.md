# U9E +1081 whole local body, bounded by four callees

The original DOS v1.3 `GAME.OVR` file is 206,105 bytes with SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit U9E's `+1081..+10E3` source occupies file `0x2BF6B..0x2BFCD`:
99 bytes, SHA-256
`a75691411c86d02b21a163db942f0564c574b6ab4f97f4cecb4b90c6c5b4c0fc`.
The bounded source walk has 41 aligned instruction starts, one conditional
edge (`+10A1` to `+10DB`), three far calls, one near call, and sole outer
`RETF 6` at `+10E1`. Nine far callers make this a callable game route. The
next code begins at `+10E4`; no code beyond the return belongs to this body.

| Original site | Local C effect or boundary |
| --- | --- |
| `+1081..+109A` | Build a 14-byte BP local frame, zero return flag, push caller arguments and SS local output pointer, then call resident `00BA:2199`. |
| `+109F..+10A1` | Test lookup `AL`; zero goes directly to `+10DB`. |
| `+10A3..+10B3` | Build destination and private `CS:+1078` source pointers; call SYSTEM `0AF8:0B0B`. |
| `+10B8..+10BE` | Push two byte-valued AX words, preserving AH, then call resident `00BA:16EC`. |
| `+10C3..+10D4` | Push the caller arguments and live lookup output words, then `PUSH CS; CALL near +0026`. The explicit CS word precedes the near return IP on the stack. |
| `+10D7..+10E1` | Set the local return flag, read it into AL, restore BP/SP, and return far with six argument bytes removed. |

The C source preserves the local instruction order, live CPU words, 20-bit
physical RAM wrapping, contiguous word bytes, stack frames, and caller-visible
outer `RETF 6`. Host callbacks own full-state effects and return cleanup of
the four callee sites. Their loaded segments are supplied, because the
source operands `00BA` and `0AF8` are relocation names rather than proven
runtime CS values. The source assumes the loaded U9E body is immutable.

The [sidecar lookup](RESIDENT_SIDECAR_LOOKUP.md) establishes the first
callee's five-word argument frame and `RETF 10`. The
[SYSTEM copy evidence](SLOT_A_NATIVE_U6F_1965_SYSTEM_COPY.md) establishes
the `0AF8:0B0B` source mapping and string-copy ABI on a separate reached
route. Those references supply ABI context only; no U9E +1081 callee or
natural caller state has been captured here. A bounded `deja` recall query
before implementation timed out with no reusable indexed session result.

The synthetic focused C test checks the zero and positive lookup branches,
all four call-entry frames, local output writes, AH preservation, and the
outer return frame. The original-byte checker locks the 99-byte source
window and call/return sites. Original-byte full-state C parity and a native
reached frame remain open under the PM host-native hold. This source gate
does not change the strict full-C count of 82.
