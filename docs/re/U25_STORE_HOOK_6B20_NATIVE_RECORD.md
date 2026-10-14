# Native class-1 `0x6B20` arithmetic store and active-record write

A controlled DOS v1.3 replay ran unit `0x1C` DIVIDE with literal numerator
21, literal divisor 3, and class-1 destination `0x6B20`. The native quotient
was 7. Unit `0x25` store-word changed the window-1 destination from sentinel
`0xBEEF` to `0x0007` at `30CF:0CC9`; its store hook then changed active
record byte `+0x18` from sentinel `0xA5` to `0x07` at `30CF:0ABA`. Paired RAM
snapshots on each side of both writes establish that order for this input.
The handler reached its post-store instruction and far-call return. This is a
controlled instruction trace, not proof that an unmodified script naturally
reaches this destination or that the full arithmetic C path matches native.

## Provenance and controlled input

The ignored original `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
the slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The debugger-enabled DOSBox-X binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
At `3582:026C`, the natural RNG seed was restored to the saved AL=13 seed;
the replay selected DIVIDE and returned from the prior native instruction at
`3582:2F73`. That baseline 640 KiB RAM SHA-256 is
`ae916ba204c16cb67aacb88a636cf56859b99cd46ce647a20f032ffb6a4a2d58`,
with next virtual script PC `0x9B71`.

At virtual PC `0x9B71`, the private eight-byte instruction in script buffer
`3AD6:0271` was replaced with DIVIDE: literal 21, literal 3, destination
class-1 word `0x6B20`. The inserted instruction SHA-256 is
`e7a25eedd859269987839ed0d66fce73a2343493c390bd4f04bac6c86832de4c`.
Window-1 word `0x6B20` was at `3A16:0040` (physical `0x3A1A0`) and was
set to `0xBEEF`. The active-record pointer at DS:`[0x5D92]` was `449A:0008`;
record byte `+0x18` (physical `0x449C0`) was set to `0xA5`. Relative to the
baseline image, these controls changed exactly 11 RAM bytes: eight script
bytes, two window bytes, and one record byte. Controlled pre-run RAM SHA-256
is `6c855bad809d1dce476a128eb6aa8c9e3ace3aeaeb6c35cbab77e676b4a4c1ac`.

The exact original and loaded code spans are distinct where DOS relocation
adjusted far-call segment words:

| Routine | Original file span and SHA-256 | Loaded RAM span and SHA-256 |
| --- | --- | --- |
| Arithmetic handler, 145 bytes | `0x0EE9..0x0F79`: `32f04c0374596652b532b94da3cd8eddfaa90108b8b3b7ed9f754c7591aa7707` | `3582:01A3..0233`: `23819e5c20e81940bd9d098583bf52e7e810555a49553d6550bbd89fdb0473f4` |
| Class-1 store hook, 515 bytes | `0x7F47..0x8149`: `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` | `30CF:0A74..0C76`: `35d2a7b223356b4fe14bceb703c0abf0cec76d5a3f92979297ba88beaf9fbec8` |
| Store-word, 405 bytes | `0x814A..0x82DE`: `3a7c7ed533244cf2eef7ab02bc9ac923f335b52099cf3e6f452fbe38f4e366c5` | `30CF:0C77..0E0B`: `3a7c7ed533244cf2eef7ab02bc9ac923f335b52099cf3e6f452fbe38f4e366c5` |

The original store-hook hash and its address case were previously established
by the [store-hook audit](ECL_STORE_HOOK_0A74.md); the arithmetic and
store-word spans were identified by the [strict parity audit](U1C_ARITH_STRICT_PARITY_GAPS.md).
This replay measured their loaded bytes and execution boundaries. The local
`deja` lookup could not acquire its read-only cache lock; these cited audits
were checked against the current run's original and loaded bytes.

## Native stops and write order

All snapshots below are complete 640 KiB RAM images kept under ignored
`resources/`; only hashes and measured values are public. The virtual script
PC had advanced to `0x9B79` before the first store breakpoint, and stayed
there through far return.

| Stop | RAM SHA-256 | Window-1 `0x6B20` | Record `+0x18` | Observation |
| --- | --- | ---: | ---: | --- |
| Controlled pre-run | `6c855bad809d1dce476a128eb6aa8c9e3ace3aeaeb6c35cbab77e676b4a4c1ac` | `0xBEEF` | `0xA5` | DIVIDE instruction installed; no result store yet. |
| `30CF:0CC9`, before window write | `817617e3fb0d0508fc601046e39b70332c4f7803872e178f7c0d73b443547810` | `0xBEEF` | `0xA5` | Store frame has destination `0x6B20`, value 7; `ES:DI=3A16:D640`, whose word-store offset wraps to `0x0040`. |
| `30CF:0CCE`, after window write | `f8f8d6c3d762c77b926418ab6411738b518f9c9d240e4b2d5cc7df4353298ad3` | `0x0007` | `0xA5` | Relative to the preceding snapshot, only the two destination bytes changed. |
| `30CF:0ABA`, before record write | `7ebecfcb8ceb604516fd782dff12d91a49cc34d40676b03681a4a1208da53820` | `0x0007` | `0xA5` | Hook has selected the `0x6B20` range case and the active-record byte target. |
| `30CF:0ABE`, after record write | `b17cf4b121bb05a0b29d022c93e3890b22018e1bf3f88ad04d6aa9b6309bd014` | `0x0007` | `0x07` | Relative to the preceding snapshot, only physical byte `0x449C0` changed. |
| `3582:2F73`, far-return target | `b17cf4b121bb05a0b29d022c93e3890b22018e1bf3f88ad04d6aa9b6309bd014` | `0x0007` | `0x07` | Handler reached `3582:0230` after the store helper, then returned with the same RAM image and script PC `0x9B79`. |

The native store-word code at `30CF:0CC9` writes the 16-bit result to the
window word, then calls the class-1 hook at `30CF:0CD5`. The hook's `0x6B20`
case uses the active-record pointer and writes the low result byte at
`30CF:0ABA`. The two single-instruction RAM comparisons above directly
observe both writes. They do not imply anything about other hook addresses
or callbacks.

## C boundary

The current `pr_rec_ecl_store_word` writes the window word and returns
`PR_REC_ECL_WINDOW_HOOK` for `0x6B20`; it does not invoke the separate
`pr_u25_store_hook_0a74` model. That standalone model has the record-byte
case, but the arithmetic path has no composed active-record state or callback
execution. This trace confirms the native effect for one controlled input;
it does not give the composed C path full-C credit or establish side effects
for unobserved hook cases. No C implementation or count changes accompany
this note. Original media, modified script bytes, debugger output, and RAM
dumps remain under ignored `resources/`.
