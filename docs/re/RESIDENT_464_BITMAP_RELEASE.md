# Bounded DOS v1.3 resident bitmap release

`pr464_bitmap_release` translates `RES_0464_2595`, the far routine at
`0x0464:0x2595`. The loader expands its entry at image offset `0x06BD5`;
the first packed source byte is START.EXE file offset `0x06C0E`.
This is a **loader-proved mapping** for the exact executable below, not a
segment-wide linear-file claim. The
[loader map](RESIDENT_LOADER_MAP.md) and
[callable ledger](resident_callable_ledger.csv) supply the mapped start and
incoming-call counts. The implementation and tests contain no original bytes.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Caller and role evidence

Eleven GAME.OVR far calls target `0x0464:0x2595`; their segment operands have
overlay fixups. The sites are unit `0x017:+0x06F8`, `0x02F:+0x005F`,
`0x033:+0x1E46/+0x1E50`, `0x068:+0x108A/+0x1094`,
`0x3B9:+0x04CA`, and `0x3CE:+0x00E1/+0x017A/+0x01A5/+0x0635`.
The unit `0x033` state-reset path releases the two DS bitmap-pointer slots
at `0x6B01` and `0x6B06`, then later calls bitmap allocation routine
`0x0464:0x248E` for another DS slot at `0x6E36` (`+0x1F0B`). Unit
`0x017` releases a local bitmap-pointer slot after using the associated
image routine at `0x0464:0x1951` (`+0x06EE..+0x06F8`). These paired paths
establish game use of the custom bitmap header and its lifetime contract.

The role is **bitmap-resource cleanup in game paths**. The ultimate source
ownership of this graphics support code remains unresolved: no stock Turbo
Pascal object-code match or source attribution is claimed. This candidate
does not increment the verified game-specific translation count before PM
review. The shorter `0x0464:0x2A0D` BIOS palette wrapper was intentionally
excluded as likely library support.

## Complete routine behavior

The entry has one far pointer to a caller-owned far-pointer slot and ends
at `0x06C6F` with `RETF 4`. It makes no near calls and only calls SYSTEM
`0x0AF8:0x0254` (the memory release helper) on non-null allocations. The
routine has 52 reachable instruction starts, three conditional branches
(`0x06BE5`, `0x06C14`, `0x06C1B`), two unconditional jumps
(`0x06BE7`, `0x06C35`), and one far return at `0x06C6F`. All observed
normal paths reach that return.

| Image addresses | Effect |
| --- | --- |
| `0x06BDB..0x06BE7` | Read the slot's offset and segment; return unchanged only if **both** words are zero. |
| `0x06BEA..0x06C14` | Reload the bitmap pointer. Read header byte `+8` and word `+0x11`; multiply as unsigned 16-bit with low-word truncation. Read auxiliary far pointer at `+0x13/+0x15`. |
| `0x06C16..0x06C4A` | If the auxiliary pointer has either word nonzero, reread it and release it first. Its size is the truncated product shifted right twice when DS byte `0x4A1E` equals one; otherwise it is the whole product. |
| `0x06C4A..0x06C6F` | Reread the caller slot in segment-then-offset push order. Release the primary block with `product + 0x17` modulo 65536. Write zero to the slot's offset word and then segment word. |

The C API uses memory and release callbacks so far-pointer words, 16-bit
offset wrap, call order, and writes remain visible. Its `video_mode`
parameter represents DS byte `0x4A1E`; the DOS routine consults that byte
only when an auxiliary block exists. On a callback failure, the C function
returns a host status and leaves preceding effects in place. Such host
failures and corrupted pointers are outside the observed DOS contract;
successful callbacks cover the complete decoded routine. The allocator's
internal heap implementation is delegated through `free_block`, matching
the original external SYSTEM call boundary.

## Reproduction

Before implementation, `deja "poolrad resident 0x464 issue 59 game routines
loader caller proof"` and `deja "poolrad 0x464 resident segment routines"`
recalled the earlier START.EXE relocation lead and resident work. This note
reuses the reviewed [loader map](RESIDENT_LOADER_MAP.md),
[resident inventory](RESIDENT_FUNCTION_INVENTORY.md), and
[callable coverage](RESIDENT_CALLABLE_COVERAGE.md); the cited start,
caller sites, and branches were checked against the hash-matched local
files. No private files enter Git.

Synthetic tests cover null and partially null pointers, absent and present
auxiliary blocks, mode `1` versus other modes, 16-bit product and offset
wrap, ordered release/clear effects, and callback failure boundaries.

```sh
cmake -S . -B /tmp/poolrad-issue59-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue59-build
ctest --test-dir /tmp/poolrad-issue59-build --output-on-failure
```

The strict build passes every CTest target on this branch. The
[issue #59](https://github.com/huntergdavis/poolrad-decomp/issues/59)
game-routine classification remains for PM review; no full-game count or
completion percentage is asserted here.
