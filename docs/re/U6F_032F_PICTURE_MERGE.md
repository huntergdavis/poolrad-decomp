# U6F +032F as a whole leaf: the picture merge over mapped memory (Claude C1, PM GO i8916)

**Scope.** The complete executable span of GAME.OVR unit 0x6F + 0x032F translated into
portable C over host-supplied memory views, with no callbacks: the routine makes no
calls. Inputs: `GAME.OVR` 206,105 bytes, sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; `START.EXE` 65,231
bytes, sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit 0x6F
starts at GAME.OVR offset 0x1DFE6 (9,189 code bytes). The routine is not a public entry;
its only callers are the two near sites inside entry 3 (+0482, the picture builder of
[U6F_0482_PICTURE_BUILD.md](U6F_0482_PICTURE_BUILD.md), whose `merge_picture` boundary
this closes). Ownership: screened read-only (i8909) and assigned by the PM in i8916; no C
of this span exists in the tree, and the replay fixture that names `[032B,032F)` covers
the +02AE return tail, a different routine. The `deja` recall hung (index damaged per
i8257); the merged notes cited below are the prior evidence. C:
`src/recovered/overlay_u6f_032f_picture_merge.c`,
`include/poolrad/recovered/overlay_u6f_032f_picture_merge.h`,
`tests/recovered/test_overlay_u6f_032f_picture_merge.c`, built by
`tests/recovered/build_u6f_032f.sh`; CMake registration in a separate commit of the same
branch.

## 1. Byte identity, independently derived

| item | value |
| --- | --- |
| executable span | `[032F,0476)`, GAME.OVR `[1E315,1E45C)`, 327 bytes, 121 instruction starts, sha256 `2df5a5980843eb72e7ed5382d4d4d452d541c22ebfe0d43ee065947b016c2dde` |
| outer return | `retf 8` at +0473 (two far-pointer arguments: the source object at `[bp+0xA]`, pushed first, and the destination at `[bp+6]`) |
| neighbours | +02AE..+032E precedes (the directory routine, merged model); `[0474,0476)` is 2 data bytes before the +0482 constants |
| frame | `sub sp, 6`: `[bp-6]` the loop bound, `[bp-4]` the loop index, `[bp-2]` the auxiliary bound in mode 1 |
| calls | none |
| branches | 15: +033A `je +0343`, +033E `je +0343`, +0340 `jmp +03C1` (the mode switch); +0353 `ja +03BE`, +0358 `jmp +035D`, +03BC `jne +035A`, +03BE `jmp +0470` (the mode-0/2 loop); +03C3 `je +03C8`, +03C5 `jmp +0470`; +03D8 `ja +0412`, +03DD `jmp +03E2`, +0410 `jne +03DF` (the mode-1 OR loop); +042D `ja +0470`, +0432 `jmp +0437`, +046E `jne +0434` (the mode-1 AND loop) |
| callers | unit 0x6F + 0x0546 and + 0x056A (inside +0482), near; no far caller (not an entry) |

Source and ownership map of what the span touches:

| item | where it is established | owner |
| --- | --- | --- |
| `DS:0x4A1E`, the display mode byte (0 selects the CGA route elsewhere; this leaf treats 0 and 2 alike, 1 separately, others as nothing) | [FONT_8X8D_201.md](FONT_8X8D_201.md), [RESIDENT_464_POSTPROCESS_MAP.md](RESIDENT_464_POSTPROCESS_MAP.md) | read through the DS view at +0335 |
| the bitmap object layout: span word at +0x11, auxiliary far pointer at +0x13/+0x15, primary data at +0x17 | [RESIDENT_464_BITMAP_CONSTRUCTOR.md](RESIDENT_464_BITMAP_CONSTRUCTOR.md), [RESIDENT_464_MODE0_PRIMARY.md](RESIDENT_464_MODE0_PRIMARY.md), [RESIDENT_464_MODE1.md](RESIDENT_464_MODE1.md) | read through the views |
| `DS:0x5EB2`, the far pointer to slot 0x0B's first picture (table `DS:0x5E5A + 8*11`) | [U3CE_ENTRY4_01C8.md](U3CE_ENTRY4_01C8.md) (the slot table), [U6F_0482_PICTURE_BUILD.md](U6F_0482_PICTURE_BUILD.md) (slot 0x0B is the temporary picture the builder loads first) | read through the DS view at +0412, after the OR pass |

## 2. Semantics

```
procedure Merge(src, dst: ^Bitmap);                   { near leaf, retf 8 }
  case DS:[$4A1E] of                                  { +0335 }
    0, 2: begin                                       { +0343 }
      n := src^.span - 1;                             { 16-bit; the "0 > n" test is unsigned and never skips }
      for i := 0 to n do begin                        { +035D..+03BC }
        dst^.data[i] := dst^.data[i] or src^.data[i];
        dst^.aux^[i] := dst^.aux^[i] and src^.aux^[i];   { both aux pointers reread from the headers each step }
      end;
    end;
    1: begin                                          { +03C8 }
      n := src^.span - 1;
      for i := 0 to n do dst^.data[i] := dst^.data[i] or src^.data[i];   { +03E2..+0410 }
      base := DS:[$5EB2];                             { +0412: read here, after the OR pass }
      m := (base^.span shr 2) - 1;                    { +0416..+042D: the slot-0x0B picture's span }
      for i := 0 to m do dst^.aux^[i] := dst^.aux^[i] and src^.aux^[i];   { +0437..+046E }
    end;
  end;                                                { other modes: +03C1..+03C5 -> +0470 }
```

What it is: the picture builder composites the temporary slot-0x0B pictures over the
party-slot pictures by ORing the primary data and ANDing the auxiliary planes (a mask) byte
by byte. In the mode-1 (four-bank) display the auxiliary plane is a quarter of the slot-0x0B
span; in modes 0 and 2 it is as long as the source span.

Facts the C preserves: the mode test routes 0 and 2 together, 1 separately, and every other
value to the return; the loop bounds are `span - 1` and `(span shr 2) - 1` in 16-bit
arithmetic and the entry tests (`xor ax,ax; cmp ax,[bp-6]; ja`) can never skip, so a span of
0, or a slot-0x0B span below 4, runs the loop for all 65,536 offsets; the index wraps at
16 bits, so every address is `segment:(offset + displacement) mod 0x10000`; the ORs happen
before the ANDs at each offset in modes 0/2 and as two whole passes in mode 1; the
auxiliary far pointers are reread from both headers at every step, so a sweep that
rewrites a header changes the later steps (section 5 pins one such case); the mode byte is
read once at entry and the slot-0x0B pointer at +0412 only in mode 1 and only after the OR
pass, so an OR pass whose destination covers `DS:0x5EB2` changes which picture bounds the
AND pass (section 5 pins that too; PM review i8993); the source
object's own bytes are never written except through aliasing; only the source span (and in
mode 1 the slot-0x0B span) is read, never the destination's; nothing is returned.

## 3. Memory model

Arguments are DOS far pointers (offset then segment); the state carries the DS paragraph,
through whose view the two DS reads (`0x4A1E` at entry, `0x5EB2` at +0412) go in source
order. The host supplies one view per segment (`segment`, `bytes`, `size`); a byte access resolves to the view of its segment
at the wrapped 16-bit offset and must lie below `size`, otherwise the model stops with
`BOUNDS` at the source offset of the access, keeping every earlier write. There is no
callback and no host guard: the longest loop is 65,536 steps. Overlapping or aliased objects
(the same object as source and destination, planes that overlap, objects whose headers lie
inside the swept range) behave as in DOS because every step reads and writes through the
views in the engine's order. `ARGUMENT` is reported only for a null state, a null output or
a null segment table with a non-zero count.

## 4. Classification

**Complete translation of the leaf over mapped memory**: all 121 instruction starts and
all 15 branches are represented, with no callback. No native differential has been run
(host hold), so no strict credit is claimed; the PM decides credit on an actual differential.

## 5. Tests

Three synthetic 64 KiB segments (objects, destination, planes). Modes 0 and 2: four data
bytes ORed and four plane bytes ANDed, bytes past the span untouched, the source unchanged,
the destination's own span ignored. Mode 1: the OR pass over the source span, the AND pass
over `(slot-0x0B span shr 2)` bytes (2 for span 9, 5 for span 20) independent of the source
span. Every other mode value 3..255: nothing read or written beyond the mode byte (with no DS view
the mode read itself is BOUNDS at +0335). Mode 1 with the destination object placed in the DS
segment so that its data covers `DS:0x5EB2`: the OR pass rewrites the pointer's segment and
the AND pass follows the rewritten pointer (a different base picture, span 8, two bytes);
the same layout in mode 0 reads no DS pointer.
Span 0 with the objects in separate segments: 65,536 OR steps copy the source segment into
the destination segment shifted by 0x100 (the source header and the base span land there)
and 65,536 AND steps through the wrap leave every plane byte ANDed. The same sweep with both
objects in one segment: the base span propagates through the wrap into the source span
word and the destination's auxiliary offset into the source's, so the plane is ANDed with
itself (pinned as engine behavior). Aliases: source = destination leaves everything
unchanged; destination plane = source plane + 1 chains the ANDs in offset order; a source
header at 0xFFF0 whose data wraps to 0x0007 and a plane pointer at 0xFFFF wrapping to 0.
Bounds: an unmapped auxiliary segment stops after the first OR with the AND count 0; a view
shorter than the header stops at the span read; an unmapped slot-0x0B picture stops after
the OR pass at the span read (+0416) with the pointer reported; a DS view too short for
`0x5EB2` stops at the pointer read (+0412); an unmapped destination stops at the first OR; null arguments. Builds: strict
(`-std=c11 -Wall -Wextra -Wpedantic -Werror`), the same under
`-fsanitize=address,undefined`, and `gcc -fanalyzer` on the model and the test (clean).
`cppcheck` and `clang-tidy` are not installed on this host.

## 6. Registration

Applied in the separate commit of this branch, after the +1262 entries:

```cmake
# poolrad_recovered sources: + src/recovered/overlay_u6f_032f_picture_merge.c
add_executable(poolrad_overlay_u6f_032f_picture_merge_tests tests/recovered/test_overlay_u6f_032f_picture_merge.c)
target_link_libraries(poolrad_overlay_u6f_032f_picture_merge_tests PRIVATE poolrad_recovered)
add_test(NAME poolrad_overlay_u6f_032f_picture_merge_tests COMMAND poolrad_overlay_u6f_032f_picture_merge_tests)
```

## 7. Queue

Next, pending PM scope: +069E (entry 4, 1,062 bytes, pinned in i8757), after a read-only
ownership screen. Not started.
