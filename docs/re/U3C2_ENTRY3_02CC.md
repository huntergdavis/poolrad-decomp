# U3C2 entry3 +02CC: complete record step

The C API pr_u3c2_entry3_02cc executes the complete original function through
RETF 4. It reads one far record pointer, chooses a signed comparison path,
conditionally updates record words/bytes, reads a DS table byte, and restores
the live caller frame. There are no calls inside the body.

The implementation is reused byte-for-byte from explicitly released pF head
6f91e653b2184f17d196a6149edde8182c6726c2 (release i8697; PM assignment i8670
and archival reuse approval i8725). Its source blob is
683c62270725879f8106e14e35f7a8049a967eee. This branch expands the contract,
independent synthetic tests and source-only audit without adding another API.

| Source fact | Measured value |
| --- | --- |
| GAME.OVR | 206105 bytes; SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Body | U3C2 [02CC,0422); file [0x2FE1E,0x2FF74); 342 bytes, 111 instruction starts |
| Body SHA-256 | 818132a151e78bff884848a874203a09a50b6d459e0ad9f5e7278de692d28337 |
| Entry/return | Descriptor slot3 is 02CC; sole 041F RETF 4; next entry5 starts 0422 |
| Calls | None; exact incoming near sites in entry5 are 0541 and 054D |
| Ledger callers | Two far and two near incoming references; no exhaustive new far-site census |
| Instruction width | Six one-byte opcode98 sites are CBW in 16-bit code, extending AL to AX; a decoder's CWDE alias does not change operand width |

The argument's LES and all subsequent local-pointer LES operations read four
physically contiguous bytes. Every record access reloads that local pointer.
Word operands wrap physical addressing at 20 bits; separately executed stack
operations wrap effective offsets at 16 bits. Code must remain immutable,
CPU storage distinct from RAM, and execution synchronous.

The function stores its direction lookup at record +19, then loads the return
AL from the live local byte. With disjoint record/stack storage that byte is
the move flag. If an aliased record write changes it, the actual live byte is
returned; the API does not force a Boolean. A record counter can also overlap
the saved local pointer and redirect later accesses. The C preserves these
effects rather than caching a record pointer or result.

Only the final ADD's arithmetic flags remain observable. The intervening
comparisons, INCs and shift flags are overwritten before return; omitting
those transient flag calculations preserves the complete final CPU state
under the synchronous/no-callback contract. Final FLAGS preserve incoming
bits outside OF/SF/ZF/AF/PF/CF and define all six arithmetic bits.

## Current focused source-only checks

Strict C11 and ASan/UBSan each pass 397112 synthetic all-14-CPU cases:
393216 cases enumerate every signed-byte pair on all six update/stopped paths,
and 3896 cases compare every byte of the complete 1 MiB RAM individually.
The bulk loops compare all CPU words and record bytes per case, then complete
RAM at the end of each of six loops. Incoming FLAGS range across every
16-bit pattern in each bulk loop; that is a raw-model domain, not a native
reserved-FLAGS guarantee.

The full-RAM matrix spans nine SP offsets, three SS values, three record/
table layouts and eight signed-byte pairs on all six paths. It covers LES
operand crossings, separately wrapped FFFE/FFFF return pops, record offset
wrap, 20-bit wrap, and record/DS-table aliasing. Six additional signed-word
comparison cases test the 8000/7FFF boundary and equality. Two literal alias
cases prove live pointer redirection and a direction write changing returned
AL to A5. Five invalid-argument calls verify atomic CPU/RAM rejection.

The mathematical test specification assumes disjoint record/stack storage.
The two literal alias cases separately test specific overlapping layouts;
they do not exhaust every possible alias. No original instruction execution,
emulator or DOSBox was run. On restack, the registered CMake target was also
built serially with strict C11/ASan/UBSan and its single CTest passed (1/1).
The broader test suite was not run.

## Attributed archival evidence

The longer target deja query completed but reported zero searched sessions;
the shorter target query timed out (exit124). Neither supplies a reusable
session finding. Concrete reused sources are the released pF head above,
the [function ledger](overlay_function_ledger.csv), and the pinned original
body/header facts independently checked in this lane.

The archival document reports 88/88 Unicorn cases matching all 1 MiB RAM
and all14 CPU words. Inspection of its case generator shows SP values 8000
and FFFB; the latter places the incoming far-pointer operand at SS:FFFF.
Its eight extra cases cover specific stack-record and DS-table aliases.
Those 88 cases do not include FFFE/FFFF return frames.

The separate historical DOSBox-X normal386 report records four synthetic
cases: SP FFFE, FFFF, FFFB and FFFB. Each compares 14 CPU words, 65 stack
bytes and 26 record bytes; it does not compare whole native RAM. The report
SHA-256 is 63f8c6b1b3645a7faf5f80ef46f246dc414f83021d9f598a3ca0c47a51bb5f6b.
Its executable SHA-256 is
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
These findings are historical, not rerun on this branch. The archival native
launcher remains at that released commit and is not added by this change.

| Review field | Assessment |
| --- | --- |
| Target | Complete U3C2 entry3 [02CC,0422), including RETF4 |
| Candidate | pr_u3c2_entry3_02cc, unchanged released C implementation |
| Independent signals | Exact body/hash/boundary, closed branches, CBW width, no calls, live LES/access ordering, final ADD flags, return frame |
| Current score | 397112 synthetic all14 CPU cases; 3896 individual full-RAM checks; five atomic guards; static pinned-body audit |
| Verdict | Likely source match with attributed archival controlled-match findings; fresh original execution and natural game identity remain unmeasured |
| Limit | Provisional source-only result; zero new strict manifest/count or denominator claim |

## Focused reproduction

Keep binaries and original media under ignored resources. From the repository root:

```sh
mkdir -p resources/u3c2-02cc-focused
nice -n 19 timeout 20s cc -std=c11 -O1 -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
  src/recovered/overlay_u3c2_entry3_02cc.c \
  tests/recovered/test_overlay_u3c2_entry3_02cc.c \
  -o resources/u3c2-02cc-focused/test
nice -n 19 timeout 20s resources/u3c2-02cc-focused/test
python3 tests/recovered/check_overlay_u3c2_entry3_02cc.py \
  --static-only --media-root /path/to/ignored/resources/DOS/POOLRAD
```

Capstone is required for the static audit. Omitting --static-only runs the
archival 88-case Unicorn path and requires an explicitly approved execution
slot while the host hold applies. Native gameplay reach and strict credit
remain separate PM gates.
