# U38 +25BC through the raw resident selector

The existing U38 +25BC caller and +101D helper used a callback for resident
00BA:2E8B. New raw CPU APIs in those same source/header files execute the
complete resident selector, compose it through +101D, and compose that result
through +25BC. The original callback APIs and their source bodies are retained
unchanged. This closes the raw CPU dependency gap without substituting a
selector result or suppressing the original stack and memory effects.

The resident routine clears a far output cell and selects the character's
+CC link or +F8/+FC alternatives according to an indexed DS flag byte. +101D
falls back to the original +CC link when the selected output is null. +25BC
uses the selected object's +2E index and DS table mask 81h to decide whether
to halve DS:6818. Every source read, word write, local, argument, LES operand
and return frame remains live.

This is author evidence for independent verification. It changes no ledger,
classification, acceptance record or credit. +25BC remains game_evidenced /
provisional and +101D remains game_or_other_library_unresolved / unresolved.

## Identity and selection

| Original span | Exact size | SHA-256 |
| --- | --- | --- |
| RES_00BA [2E8B,2F57), expanded image [3A2B,3AF7) | 204 bytes / 69 instructions | c7aa0a429219bbc7e162996a4af3e1a8e1c95bad8b847edcd0244e42ed54af67 |
| U38 [101D,1086), GAME file [FC78,FCE1) | 105 bytes / 36 instructions | 632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a |
| U38 [25BC,2605), GAME file [11217,11260) | 73 bytes / 29 instructions | 4921365d3d512d870ac24085a63d408b38ab60e6f98e4136f583a7c5c78fe753 |

START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The loader inventory pins U38 at file EC5B and the +1048 resident-segment
fixup. The only calls are +25CB -> +101D and +1045 -> resident +2E8B; the
resident body is call-free. Returns are RETF 0Ah, RETF 4 and RETF 8,
respectively. Total translated closure: 382 bytes, 134 instruction starts.

The schema-4 queue lineage emits schema 8. On the selection snapshot, +25BC
is rank 392, score 18, with its +101D dependency labeled modeled/ready.
+101D is rank 465 and labels the resident dependency resident_covered.
Those labels did not establish raw composability: both main APIs still
required the selector callback. The higher substantive ready rows were
already raw implementations. The other selector callers can now use the
new +101D API in subsequent author work.

The queue's +25BC extent is 84 bytes / 34 decoded instructions. Its final
11 bytes [2605,2610) are data, not part of the function. Their SHA-256 is
095a90f713555df4530f7b65abc167913bbd989a9ac89489bc059ee02adf516d.
This implementation ends after RETF at 2602. An initial audit assertion
mistakenly expected 28 prefix instructions; direct decoding established 29,
and the count was corrected before the parity run.

## Reused evidence and ownership

The preimplementation memory query was "poolrad RES_00BA_2E8B raw CPU target
selector ownership U38 101D". It recovered the prior callback boundary and
U38 helper work (Codex session 01a1386e-bea7-7883-8ddd-06fd2be8db9b), not a
raw selector implementation. The VP confirmed ownership and required the
existing files to be extended (inbox i18559).

Historical branches were inspected rather than treated as current code:

- resident selector: 3f360238720885eb103b553f6e03711c75ebc331;
- U38 +101D helper: 8ee237f0c7bf791c40345b4e15acda5d1501d623;
- U38 +25BC caller: 8c0a7e7977f3eb022195632155603894a1f7cbc2.

Their source identity, field order and semantic expectations are reused.
The original-byte audit executes all three real bodies. The new native
harness uses the disk-fed capture structure from the +2E08 audit and loads
actual resident code, with its far-call binding relocated to segment 7000h.
No resident callback or synthetic result stub is used in either oracle.

## Raw API contracts

The new public functions are pr_res_target_select_2e8b_cpu,
pr_u38_target_select_101d_cpu and pr_u38_target_mask81_25bc_cpu. Each takes
CPU14 and exact 1 MiB A20-off flat RAM. CPU/diagnostic storage must be distinct
from RAM and one another. Effective offsets wrap at 16 bits; bytes within
words and LES/far-return operands are physically contiguous with A20 wrap.
Code is immutable and mapped without overlap with mutable operands. Reached
far-return frames must fit in SS:0000..FFFF. Interrupts, MMIO, faults and
self-modifying code are outside the profile.

| Entry | Accepted entry SP | Stack arguments after return IP/CS |
| --- | --- | --- |
| Resident 2E8B | 0008..FFF4 | far output cell, then far character pointer |
| U38 101D | 0020..FFF8 | far character pointer |
| U38 25BC | 002E..FFFC | ten unused bytes |

IP must match the selected entry. Invalid host arguments reject atomically,
including the optional diagnostic. The diagnostic initializes both fields
to -1 on valid entry, then retains the raw resident and selector results.
The near-call opcode/displacement and far-call opcode/offset are checked at
the actual CALL boundary; the resident segment operand is read live.

RETURNED retains the actual outer RETF destination. CODE stops before the
unsupported CALL, preserving preceding argument pushes. CALLEE preserves a
nonzero dependency result and its CPU/RAM. TRANSFER preserves a successful
callee's unexpected return destination. The observed partial cases exercise
CODE directly and its propagation through CALLEE. TRANSFER and impossible
host-argument failures inside this fixed valid chain are defensive boundaries,
not additional claimed measured cases. All bodies are finite and need no cap.

## Explicit SHR AF profiles

+25BC ends its halving path with SHR AX,1. AF is architecturally undefined
there. The old callback-era native audit masked AF on that path. The new raw
API instead requires one of two named policies:

- PR_TARGET_SHIFT_UNICORN: AF is cleared, matching Unicorn 2.1.4.
- PR_TARGET_SHIFT_DOSBOX386: AF is set, matching DOSBox-X normal/386.

No implicit default, flag mask, or oracle-state rewrite is used. The policy
changes only AF when SHR executes. A path that skips halving uses the same
complete flags under either policy. Native case 200 starts with FLAGS=0002,
uses amount 18, and returns 0016; the Unicorn profile returns 0006. Thus the
native policy sets AF rather than preserving its incoming value.

The difference was found in the new capture, then corroborated by the memory
query "poolrad DOSBox SHR AF undefined carry flags shift normal 386 profile"
and the prior [allocator flag-profile evidence](SYSTEM_GETMEM_308_SELECTION_GATE.md).
The resident and +101D need no shift policy: intermediate shift flags are
replaced before observation. Their final logical AF uses the zero convention
measured by both oracles. These policies describe the audited emulators, not
all historical x86 processors.

## Measurements and limitations

| Check | Result |
| --- | --- |
| Original-byte Unicorn, CPU14 + all 1,048,576 RAM bytes, unmasked | 852 cases; explicit Unicorn SHR policy |
| Entry distribution | resident 283, +101D 283, +25BC 286 |
| Original stops | resident 283 returns; +101D 281 returns / 2 CODE; +25BC 282 returns / 2 CODE / 2 CALLEE(CODE) |
| Complete closure coverage | 134/134 instruction starts and 24/24 conditional outcomes |
| DOSBox-X normal/386 | 288 synthetic returns, 96 per entry; explicit DOSBox SHR policy |
| Native comparison | CPU14 + 1,024 stack + 65,536 DS + 65,536 heap bytes, unmasked |
| ASan + UBSan semantic assertions | 40,726 passed |
| Focused CTest, including all three legacy callback tests | 4/4 passed |

Original cases cover all 256 table flag bytes at each entry, null links and
alternatives, exact flag byte 0Ah, selection priority, index 255, wrapped
record fields, partial far pointers, random RAM, A20 stack/data aliases,
relocated resident code, odd/minimum/maximum SP, standalone output aliases
with record fields/locals/return/argument words, caller stack-record aliases,
and changed call opcodes/targets. No instruction or conditional outcome is
omitted from the decoded denominator.

Semantic tests independently evaluate selection priority and fallback over
6,144 entry/flag/variation combinations, verify selected pointers and halving,
check partial stops/atomic rejection, and compare both complete flag policies
for all 256 amounts on halving and non-halving paths. Existing callback source
prefixes are byte-identical to the tested base; their tests pass unchanged.

Native RESULT.BIN SHA-256:
4b8cc952dbecf5461a79f935f9292b95240d9a6b1a7faeb3805a2567b96741ca.

The initial native harness loaded the resident prefix at 00BAh, inside DOS
low memory, and did not produce usable evidence. That failed run is retained
privately. The corrected harness loads at 7000h and patches only the declared
resident segment binding. Its first comparison exposed the SHR AF difference;
that failure is also retained. The final run uses the explicit policy and
compares full flags. Native data is synthetic and covers normal returns only:
no gameplay, whole-RAM native parity, native A20/wrapped-frame or call-refusal
claim is made. Media, generated code, captures and receipts stay in resources/.

## Reproduction and author assessment

With private pinned media and Capstone/Unicorn available:

    PYTHONPATH=resources/pythondeps:tools/overlay python3 tools/overlay/u38_target_selector_cpu_audit.py --unicorn --native --output resources/final-oracles.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target test_u38_target_selector_cpu
    ctest --test-dir resources/build -R '^u38_target_selector_cpu$' --output-on-failure

- target_function: OVL_0038_25BC, with complete OVL_0038_101D and
  RES_00BA_2E8B dependencies.
- source_candidates: raw APIs added to the three existing selector files;
  original callback implementations preserved.
- signals: pinned complete bodies/call map; live CPU/RAM ABI; all original
  closure starts/outcomes; direct native-to-C comparison; independent
  selection/halving and explicit flag-policy assertions.
- score: 5/5 author evidence categories (identity, ABI, differential, native,
  semantics), not an acceptance or credit score.
- verdict: likely_match under the explicit RAM and flags profiles.
- review_notes: provisional/unresolved overlay identities; queue callback
  readiness caveat; 11 trailing data bytes; emulator-specific SHR AF;
  synthetic native scope; independent verification/credit remains with VP.
