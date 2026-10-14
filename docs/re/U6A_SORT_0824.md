# GAME.OVR unit 0x6A:+0824 complete C translation

## Source and extent

The owner-supplied `resources/DOS/POOLRAD/GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The function occupies file bytes `[0x18055,0x180F0)`, 155 bytes, SHA-256
`c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7`.
Its 57 decoded instruction starts cover the entire reachable body through
`RETF` at `+08BE`, with no calls. Its near caller at unit `0x6A:+0A4E`
precedes the call with `PUSH CS`, after writing a byte at offset `+17` from
the far pointer in `DS:[5D92]`. The caller does not establish a public name
for the data structure or its owner.

The candidate originated in the merged
[function alias audit](FUNCTION_ALIAS_AUDIT.md), whose snapshot identified it
as a reachable, unclaimed full-C candidate. The current assignment and claim
were checked separately before work began.

## Behavior

The function performs a selection-like sort on 21 consecutive bytes at offsets
`+17` through `+2B` from the far pointer in `DS:[5D92]`. It orders the low
seven bits ascending. A comparison swaps the *whole* bytes only when the
left key is greater. The algorithm is not stable for equal keys. It reads the
far pointer again on every `LES`, rather than caching it. The C model follows stack-frame
writes, byte accesses, register results, arithmetic flags, and `RETF`.

The implementation makes no claim that those 21 bytes are inventory, stats,
or any named game record field. No real native call/return frame has yet
identified gameplay reachability or the pointer's owner. Those facts remain
unmeasured even though the static caller is aligned.

## Reproduction

Run the original-byte audit locally, with private original media present:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_overlay_u6a_sort_0824.py
```

The audit checks the pinned file and body hashes, instruction extent, and
internal branches, then executes the original bytes under a 16-bit emulator
and the C translation from identical input states. Sixteen synthetic cases
cover sorted and reverse order, equal and masked keys, high bits, stack and
array offset wrapping, a shared DS/SS segment, and a list overlapping the
far-pointer table. Each compares the full 1 MiB RAM image and every returned
16-bit CPU register, including all defined
arithmetic flag bits. All 16 cases pass.

The raw-memory CPU interface and original-byte audit pattern follow the local
unit `0x7F:+05E0` work in [PR #387](https://github.com/huntergdavis/poolrad-decomp/pull/387);
the source span, behavior, and cases here are independent.

The CTest smoke test covers input validation, the sorted result, frame/return
state, and returned flags. Neither test establishes native route reachability.
