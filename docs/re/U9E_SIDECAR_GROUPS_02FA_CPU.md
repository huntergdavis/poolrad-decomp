# U9E +02FA: raw grouped sidecar composition

pr_u9e_sidecar_groups_02fa_cpu implements the complete caller body
[02FA,0ABB), composing the raw +0160 interface merged in PR #1042.
No-hit groups return normally. Any handler hit propagates PENDING at
U9E +001E, before the indirect CALL, with the live CPU, RAM, handler
pointer and prepared stack. No handler effects or subsequent group calls
are synthesized. This is partial composition; strict readiness and
full-chain closure remain open. No credit or accounting changes are made.

## Identity and provenance

- OVL_009E_02FA, GAME.OVR file [176612,178597): **1,985 bytes,
  911 instruction starts**, terminal RETF 6 at +0AB8.
- Body SHA-256:
  9ce9d83e68edd4ddf42dbf1215f0e6f0450219a64b948c0604f39d6cc60a4bc5.
- The next public entry is +0AE7: the 44 intervening bytes are outside
  this body and are not counted as implementation or coverage.
- 20 groups, 140 near CALL sites to +0160, 97 distinct immediate types;
  no far-call fixups inside the caller body.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

The implementation reuses memory/flag primitives and the CPU, layout,
limits, status and diagnostic contract from
[raw +0160](U9E_SIDECAR_0160_CPU.md). Both audit harnesses extend that
accepted original-byte differential and native capture machinery. The
original dependency bytes and resolved bindings are pinned by the audit.
A fresh recall query for this caller returned no matches (zero indexed
sessions searched); no prior-session claim is used as evidence. Earlier
recall trouble was followed by deja-health repair.

## Behavior and contract

The three argument words after the far return pair are member offset,
member segment, and group. Only the group low byte is tested. Values
0..19 select an ordered series of +0160 calls; 20..255 return without a
sidecar call. The caller reserves four local bytes and repeatedly reloads
member operands from the live frame. Every type assignment writes AL,
leaving AH intact before PUSH AX. It uses PUSH CS followed by near CALL,
so +0160 sees its original far-return frame.

Every successful +0160 return continues at the exact caller instruction.
Each call receives fresh supplied limits; there is no outer loop budget.
Inherited non-success statuses and diagnostics propagate without alteration.
A reached outer CALL validates both E8 and its relative operand before
execution. CODE preserves the pre-call state, TRANSFER an unexpected
callee return, and a normal outer RETF 6 uses the actual stack frame.

The API requires exactly 1 MiB A20-off RAM, IP02FA, SP[0196,FFF6],
positive chain/line/sort budgets, and all inherited +0160 restrictions.
The lower stack bound leaves +0160's required 0186 after sixteen bytes
of caller frame/arguments. CPU, layout, limits and diagnostic storage are
distinct from RAM and each other. Original code/bindings are immutable;
data and stack aliases remain live. Argument rejection leaves CPU/RAM
unchanged and resets the optional diagnostic.

The nominal 97-type list is not a complete dynamic-binding proof. Live
aliases, table contents and downstream handler effects still require an
evidence-backed contract. This implementation does not promote the
[handler inventory](u9e_dispatch_handlers.json) or enclosing callers to
strict full-chain readiness. It cannot resume itself from a PENDING PC.

## Validation

The original-byte Unicorn differential passed **593 cases**, comparing
CPU14, all **1,048,576 RAM bytes**, stop status and callee diagnostic.
It covered **911/911 caller starts, 40/40 conditional outcomes and
140/140 group CALL sites**. Reached composition totals are 2,190 starts
and 185 conditional outcomes; uncovered transitive sites remain in the
report, so this is not complete dependency coverage.

Results: **273 normal returns, 151 CODE, six CALLEE, 161 PENDING,
and two LIMIT stops**. The corpus includes all groups and invalid group
bytes, each nominal direct-hit call site, malformed outer and inherited
bindings, exhausted budgets/cycles, odd and endpoint stacks, A20 wrapping,
data aliases and deterministic random cases. No direct TRANSFER case
is claimed.

The only permitted Unicorn flag differences are inherited exact FLAGS
XOR0010 with C AF=1: **161 PENDING cases and two CODE checkpoints at
+022A/+0250**. All other flags, registers and memory match exactly.
The comparison never patches emulator state or applies a broad mask.
The native run below independently checks these boundary flags.

DOSBox-X normal/386 passed **77 synthetic original-code cases**:
**29 returns, 45 PENDING boundaries and three CODE checkpoints**.
Each compares **CPU14, SS1KiB, DS64KiB and node-segment64KiB** with
**zero flag masks**. Cases include every group's empty/first/last hit,
all four fallback types in modes 0/5, six invalid-group flag profiles,
two inherited binding checkpoints and outer CALL +0311.

Boundary captures use a far JMP replacing the unexecuted CALL at +001E,
+022A, +0250 or +0311. It changes no general register, flag or stack byte;
the recorded CS:IP names the original boundary. For outer +0311 the five
trap bytes also span the following instruction, which is never reached
on that trapped path; original bytes are restored before the next case.
The original dependency far CALLs use explicit resolved body bindings.
No dynamic handler executes, and synthetic native execution does not
establish natural-game reachability.

Native RESULT.BIN SHA-256:
db584efcda4e067e6f8a33027acf4c9170a36100a5a9179e30a9fb0cab263931.

Strict C11 ASan/UBSan passed **83 semantic checks**. Focused CTest passed
**1/1 after rebase**. Tests exercise every empty group, an invalid group
with a full-RAM check, a late type hit with exact pending frame, fresh
per-callee budgets, stopped lookup/party walks, malformed near/far calls,
live handler bindings, optional diagnostics and argument rejection.

Integration base: e46c37261a647b00d1f83fec33d6939e975d8065.
All 14 compiled sources and their headers are unchanged across rebase.
Of 41 source/header/audit pins, only resident_inventory.py changed: main
classified unrelated resident 0899:0104 and adjusted its counts. Fresh
source extraction reproduced all target/dependency identities exactly.

The private receipt resources/02fa-evidence.json pins 43 public files,
34 private artifacts, media and tool binaries. Original/native reports,
capture files and sanitizer/CTest logs are included. Documentation is
excluded to avoid self-reference. Receipt SHA-256:
042e53caac472fe129c181ca256b95015614995f3055ef534e35588578a0d978.

## Reproduction

Owner media stays under ignored resources/DOS/POOLRAD. The original
checks require Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler.

    python3 tools/overlay/u9e_sidecar_groups_02fa_cpu_audit.py --unicorn --native --output resources/02fa-run.json
    cmake -S . -B resources/02fa-build
    cmake --build resources/02fa-build --target poolrad_overlay_u9e_sidecar_groups_02fa_cpu_tests
    ctest --test-dir resources/02fa-build -R '^poolrad_overlay_u9e_sidecar_groups_02fa_cpu_tests$' --output-on-failure

## Author review

- target_function: OVL_009E_02FA complete [02FA,0ABB), through the
  inherited U9E +001E pre-handler boundary.
- source_candidates: overlay_u9e_sidecar_groups_02fa_cpu.c and the raw
  +0160 dependency chain pinned by the original audit.
- signals: exact body identity, all caller starts/outcomes/call sites,
  source-order live operands, CPU14/full-RAM comparison and separate
  native original execution without flag masks.
- score: strong evidence within the partial composition contract;
  dynamic handler effects and binding invariants remain unresolved.
- verdict: likely_match for implemented paths; partial chain.
- review_notes: no self-credit or full-chain acceptance. Independent
  review and credit belong to the reviewer/VP.
