# U3C2 entry 1 +0000: complete signed-word sign helper

The complete 46-byte DOS v1.3 GAME.OVR function returns FFFF for a negative
signed word, zero for zero, and one for a positive word. It executes its
prologue, local write, saved-BP restoration and RETF 2. There are no calls.
The C implementation is reused byte-for-byte from the explicitly released
pF archival head 6d762a86b7a18e09a1ba978c8d2b406370f0dd6d; this branch extends
its contract, focused tests and source-only audit rather than adding a second API.

| Source fact | Measured value |
| --- | --- |
| GAME.OVR | 206105 bytes; SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Body | File [0x2FB52,0x2FB80); U3C2 [0000,002E); 46 bytes, 17 instruction starts |
| Body SHA-256 | 5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf |
| Boundary | Entry slot 1 is 0000; sole return 002B RETF 2; no calls |
| Descriptor | START.EXE image offset 0x3C28 (paragraph 3C2 plus 8), after 512-byte MZ header; code file base 195410 |
| Ownership | pF explicitly released its archival candidate; current-main filename/manifest screen found no merged whole-entry-1 implementation |

The independent source audit reads the actual descriptor rather than assuming
a paragraph-aligned header. All branch destinations are within the body.
The raw-RAM API models 20-bit physical addressing; a word's high byte is
physically contiguous. RETF pops IP and CS at independently wrapped 16-bit
stack offsets. Final SP is entry SP plus six modulo 65536. CPU storage must
be distinct from RAM; code must remain immutable and execution synchronous.

The last CMP retains parity of the argument's low byte, not parity of the
returned sign. The zero path executes XOR AX,AX. Its architecturally undefined
AF is modeled as zero; no native AF guarantee follows from that choice.

## Current source-only verification

The standalone strict C11 and ASan/UBSan tests each pass 131426 synthetic CPU
cases. They cover every input word (65536 cases), every incoming FLAGS word
with eight representative inputs (65536 cases), and 354 complete-1-MiB-RAM
edge cases. Every case compares all 14 CPU words. The edge matrix covers
11 stack offsets including FFFE/FFFF, four SS values including FFFF, and eight
inputs; two additional regressions distinguish wrapped CS pops from a linear
four-byte return frame. Five invalid-argument calls verify atomic rejection.
Bulk exhaustive loops compare complete RAM at the end of each loop; individual
full-RAM comparisons apply to the 354 edge cases. Eight literal flag vectors
anchor the independent arithmetic/parity expectation.

These are C-versus-independent-synthetic-expectation checks. No emulator,
DOSBox, native game capture, broad build or CTest run was performed for them.

## Reused original-byte and native findings

The bounded target recall in this lane timed out (exit 124); it is incomplete.
The following concrete archived sources supply the reused findings:

- pF head 6d762a86b7a18e09a1ba978c8d2b406370f0dd6d reports 60/60
  Unicorn original-byte cases comparing all 1 MiB and all 14 CPU words.
  Its SP values are 8000, 9001 and 0000. It does not test FFFE/FFFF.
  This is attributed historical evidence, not a fresh run on this branch.
- Prior repaired head f936bf4, document
  docs/re/U3C2_SIGNUM_0000.md, records DOSBox-X original-byte RETF 2
  boundary captures: 16 cases, all 14 CPU words and 104 frame bytes.
  These establish separately wrapped IP/CS pops at FFFE/FFFF. The result
  SHA-256 is 145afb3a1e76a17b5884be471d43d527093f36401f65ff23d598e2c8a5ea09a0.
  Two zero-path cases mask undefined AF only. This is prior native frame
  evidence, not whole native RAM or a new capture.
- The same repaired head records 776 original-prefix native cases comparing
  14 CPU words and 64 frame bytes, plus 2774 emulator cases. Six high-SP
  emulator cases use explicit native-supported CS expectations because
  Unicorn 2.1.4 reads those return frames differently. Do not describe those
  six as raw Unicorn CS agreement.

The candidate matches the decoded control flow and previously measured
RETF policy. Fresh original-byte execution remains held for PM scheduling.
Natural saved-game reach, native A20 behavior and whole-game parity remain
unmeasured. This change makes no strict manifest/count or denominator change.

## Focused reproduction

From the repository root, keeping binaries under ignored resources:

```sh
mkdir -p resources/u3c2-0000-focused
nice -n 19 timeout 20s cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
  src/recovered/overlay_u3c2_sign_entry1_0000.c \
  tests/recovered/test_overlay_u3c2_sign_entry1_0000.c \
  -o resources/u3c2-0000-focused/test
nice -n 19 timeout 20s resources/u3c2-0000-focused/test
python3 tests/recovered/check_overlay_u3c2_sign_entry1_0000.py \
  --static-only --media-root /path/to/ignored/resources/DOS/POOLRAD
```

The static checker requires Capstone and performs no emulator import, C build
or execution. Omitting --static-only runs the historical 60-case Unicorn
checker and requires an explicit PM execution slot while the host hold applies.
Original media and all extracted bytes remain under ignored resources.
