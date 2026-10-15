# U3C2 entry4 +057B: entry-to-first-branch prefix only

The C model implements four instructions at +057B..+0584 and stops BEFORE
+0585 JL +05B1. It does not execute that branch, either continuation, the
remaining entry4 body or RETF. It adds zero strict full-C credits and makes
no natural-game or native-frame claim. The +0909 caller route remains a
separate follow-on after this exact prefix is reviewed.

| Artifact | Scope / SHA256 |
| --- | --- |
| Owner-supplied ignored GAME.OVR | 206105 bytes; 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| PM-pinned entry4 body | file [0x300CD,0x3045B), 910 bytes; 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae |
| Implemented prefix | file [0x300CD,0x300D7), offsets [0x057B,0x0585), 10 bytes; a736c9b7c7903f828f6e9650eb3764471652435344933d60895c15ba95005802 |

Original media and static decode products stay ignored under resources/.
The public oracle reads the hash-locked span but decodes/loads only the
prefix plus its unexecuted stop instruction. Its instruction-count bound
and hook stop before the first branch. It never runs the rest of entry4.

The entry snapshot has IP=057B after CALL. The model saves the incoming BP
word at the decremented stack pointer, sets BP, allocates four untouched
local bytes and compares byte SS:[BP+0E] with zero. The operand is therefore
at entry SS:[SP+0C]. The final IP is 0585; CS and all other nonmodified
registers retain their entry values. CMP defines CF/AF/OF as zero, PF from
the operand's low-byte parity, ZF from zero and SF from its sign bit.
The preceding stack-allocation arithmetic flags are overwritten by CMP.

SP and effective offsets wrap at 16 bits. The saved word's second byte is
the contiguous physical byte with A20 wrapping, including physical FFFFF
to 00000. Only those two saved-BP bytes may be written; local allocation
does not initialize memory. RAM must provide one full 1MiB image and code
must remain immutable. Asynchronous/debug exceptions are outside scope;
oracle vectors use TF=0. Invalid pointers, size or starting IP are rejected
without changing valid supplied RAM/CPU objects.

Validation uses C11/Wall/Wextra/Wpedantic/Werror and ASan+UBSan on 760
synthetic full-RAM/CPU snapshots: every input byte, independent flag
expectations, stack/offset/A20 wraps including a last-byte word straddle,
and unchanged-memory/argument checks. The original-byte synthetic oracle
compares every byte of 1MiB and all 14 CPU words on 434 quick cases,
including all 256 byte values, flag vectors, wrapped stacks, code-read
aliases and seeded RAM. It covers 4/4 prefix sites with zero flag masks,
zero emulator substitutions and zero conditional branches executed.
Both signed compare-state classes are reached; neither branch path is run.

With Capstone 5.0.7 and Unicorn 2.1.4 installed:

```sh
python3 tools/overlay/u3c2_entry4_prefix_057b_audit.py --quick \
  --output resources/u3c2-057b-prefix/quick-parity.json
ctest --test-dir resources/build-u3c2-057b-prefix \
  -R '^poolrad_overlay_u3c2_entry4_prefix_057b_tests$' --output-on-failure
```

Parity report: target_function is OVL_03C2_057B restricted to [057B,0585);
source_candidates is src/recovered/overlay_u3c2_entry4_prefix_057b.c.
Signals are the pinned hashes, exact four-instruction/stop shape, stack and
byte-operand side effects, exhaustive byte coverage and complete RAM/CPU
differential observations. Score is 4/4 sites and 434 oracle cases, not a
probability. Verdict: match within the documented prefix/domain; the full
routine remains unclear. Review notes: no downstream control-flow/body
coverage, no return behavior, no reachable native frame measured, and no
game/library identity or strict credit inferred from this prefix.

Bounded deja returned no completed finding. Reused the shared 14-word CPU
type and full-RAM parity conventions from reviewed U3C2 signum f936bf4,
plus the PM-pinned body hash and address metadata. The preexisting RETF
wrap issue is irrelevant to this prefix because no return is executed.
