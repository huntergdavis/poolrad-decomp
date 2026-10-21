# U9E +115C: character byte-pair replacement with the resident +0BB8 chain

This author candidate translates public U9E entry 18 and composes its actual
U9E +10E4/+110D and resident 00BA +0BB8 callees. The resident implementation
contains all eleven directly reachable bodies. There are no semantic callbacks
or handler stand-ins. Independent review decides identity and strict credit.

## Original source and bounds

Hash-pinned DOS v1.3 inputs: START.EXE, 65,231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd;
GAME.OVR, 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The checker reconstructs overlay descriptors and unpacks the resident image
before inspecting body hashes and instruction starts.

U9E +115C occupies **[115C,1287), 299 bytes / 109 instructions**, GAME file
[180294,180593), SHA-256
e122e18cccee13655b750e38c4d5e6eb664fca16b125cc72bcd724580af6fb8b.
Its only far relocation is the segment word at +1266. RETF 12 starts at
+1284. Public entry 18 resolves through original stub 00B0:007A.
The 189-byte [1287,1344) suffix is following-routine data, excluded as
established by [the ledger repair](QUEUE_METADATA_REPAIR.md). Its twelve
Pascal strings and one set are not executable coverage.

The resident closure has 3,153 executable bytes / 1,134 instruction starts:
+0000, +01FB, +0282, +03A0, +0BB8, +10E6, +1168, +11EA, +1282, +1325
and +13AB. Calls inside it are direct near calls with original PUSH CS and
RETF frames. The checker pins every body's original span and SHA-256.
The two existing U9E leaves add 120 bytes / 46 starts. Complete scope:
**3,572 bytes / 1,289 instruction starts**.

## Game ownership

The caller writes character fields +10/+16, walks the +7F/+81 sidecar chain
through +5/+7 links, and selects kinds 12/38 with payload +3 below 128.
The same field layout, type selection and encoding occur in reviewed
[U38 +04BA](U38_PAIR_04BA.md). The [character loader](../formats/DOS_CHARACTER.md)
establishes the owning 285-byte object and runtime pointer slots; the
list/dispatch contract is documented by [U9E +0026](U9E_ENTRY2.md).
The pair's precise gameplay name is unnecessary for this ownership argument.

Four incoming far calls are fixup-backed: U38 +3026 and U86 +130E, +2018,
+2EFE. The original U38 installer writes slot 131 at DS:6A34 as 0041:0250,
public entry 112 -> U38 +2F57. That handler forwards its member pointer
into +115C at +3026 with pair 18/100. Both installer and public-entry
mappings were rechecked in feasibility packet i19056; see the
[installer contract](U38_EFFECTS_INSTALL_31A2.md) and
[ownership method](U38_SELECTOR_OWNERSHIP_PACKET.md).
These are static ownership facts, not a naturally reached saved-game frame.

## ABI and behavior

The API receives CPU14 after CALL. SS:SP holds return IP/CS; SP+4 is the
result far pointer, SP+8 the secondary word, SP+10 the primary word, and
SP+12 the member far pointer. Only the low bytes of the two word arguments
are used. Twelve local bytes are reserved. Both existing U9E leaf bodies
share the caller's loaded CS.

When the requested primary is lower than the current member byte, or the
requested primary is 18 and its secondary is lower, the routine encodes the
requested pair, adds 128, stores its low byte through the result pointer,
and returns AL=0. The member pair is unchanged for disjoint state; aliases
retain their original ordered effects.

Otherwise it searches for the first eligible sidecar. If found, the old
member pair is encoded into that node's payload with the high bit set, and
the node's old payload is decoded into the live old-pair locals. It writes
the requested pair to the member, calls the complete resident +0BB8 chain,
encodes the retained old-pair locals through actual +10E4, writes the
result, and returns AL=1. Restored BP, return IP/CS and RETF 12 cleanup
come from the actual frame.

Resident +0BB8 recalculates fields using the member's +C8/+CA list, its
63-byte linked records, and DS lookup tables. All original clears, loops,
table accesses, arithmetic, calls and writes remain. Field names are
conservative. Exported resident entry APIs support direct boundary checks;
the composed caller always runs the full dependency chain.

## Execution contract

RAM is exactly 1 MiB with A20 disabled. CPU and budget storage are outside
RAM and each other. Code and loaded bindings are immutable caller
preconditions. Data, tables, locals and argument pointers remain live and
may alias. Effective offsets wrap at 16 bits; a word or LES operand reads
physically consecutive bytes after that calculation. RETF pops IP/CS
separately with wrapped SS offsets.

The caller verifies its immutable +1263 far-call opcode, +0BB8 offset and
supplied resident segment before mutation. ARGUMENT rejects invalid host
inputs/binding atomically. Each caller/resident instruction consumes one
shared budget unit. The finite existing U9E leaves are not charged.
LIMIT preserves earlier effects and the next unexecuted IP; no mid-call
resume is promised. TRANSFER preserves unexpected callee returns.
Cyclic lists can stop without inventing a normal return.

The C contains literal source-ordered operations and control flow, not a
runtime original-byte decoder. Arithmetic uses the DOSBox-X normal/386
profile. Logical AF is zero; undefined shift/multiply flags require the
native corroboration reported below. No checker may erase differences by
masking FLAGS.

## Reuse

Owner/VP i19171 authorizes the chain, beginning with +115C. Packet i19056
and repair #1142 supply the body/tail distinction and caller argument.
Dunking Sheep recall located Claude session
281c8700-5acc-438b-b93d-c061e28437eb as a reconnaissance lead; decisive
spans/bindings are rechecked here. Actual merged [+10E4](U9E_ENTRY16_10E4.md)
and [+110D](U9E_ENTRY17_110D.md) implementations are reused unchanged.
The checker adapts compilation, hashing and fixture machinery from the U38
+04BA and U86 installer audits. An independent source review maps all 109
caller starts and five calls, including alias ordering, partial registers,
frames and budgets; it supplements execution evidence rather than replacing it.

## Validation and limits

The final default audit passes **3,441 comparisons**: 881 entered at U9E
+115C and 2,560 standalone resident-helper cases. Caller-origin fixtures
alone reach **1,289/1,289 starts and 300/300 feasible branch outcomes**;
the aggregate has the same complete coverage. Of 314 syntactic branch
outcomes, 14 are proved impossible below. The U9E +115C body itself reaches
109/109 starts and 18/18 outcomes.

Native validation passes **33 captures**: 14 completed U9E calls, 14 completed
resident entries, and five explicit instruction-boundary checkpoints.
Wrapped SP, low SP, selected sidecars, inventory paths, and the normal/386
SHL/MUL boundary behavior are included. All compared register words and
captured RAM bytes agree without masks.


The strict original-byte differential compares all 14 CPU words, including
unmasked FLAGS, status, remaining budget, and **every byte of 1 MiB RAM**.
It makes no CPU substitutions and replaces no original callee. The default
gate requires every decoded instruction start and every feasible conditional
branch outcome both in the aggregate and through U9E +115C entry fixtures.
Caller-entry coverage includes intentional LIMIT cases; it does not mean that
every fixture completes a call.

Fourteen syntactic taken edges are impossible because of the original range
checks. The checker enumerates each site, destination and contradiction:
021E->0236, 0239->024B, 03D7->03EB, 03EE->03FB, 03FE->040B,
0C8E->0CB3, 117D->1189, 118B->119E, 11F9->120B, 1222->122E,
1230->123C, 123E->124A, 124C->1258 and 1264->1279.
It fails if any supposedly impossible edge is observed. Coverage denominators
come from hash-pinned original bytes, not ledger extents.

Native corroboration uses DOSBox-X normal/386, executable SHA-256
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
The fixtures install original bodies with the actual far segment relocation
and synthetic direct-entry frames. They do not run the game loader or
demonstrate natural saved-game reachability. Each native capture compares
five complete 64 KiB stack/data regions, **327,680 bytes**, rather than
claiming full-memory native coverage.

Completed native calls capture 13 register words and use the executed
return/capture label for IP. Budget checkpoints capture 12 register words
and declare the intercepted original CS:IP. For the latter, a far JMP
replaces the first unexecuted instruction solely to enter the capture stub;
the script asserts this is the boundary's first visit. This substitution is
explicit and is not a completed original call. Register capture precedes
flag-changing DOS output, and PUSHF uses a separate capture stack outside
the observed data pages.

Raw Unicorn disagrees with normal/386 at some standalone or budget
boundaries: undefined SHL AF, undefined MUL PF, and far returns whose CS
pop wraps SS:SP at FFFE/FFFF. These are recorded as oracle disagreements,
not masked, patched, or counted as passing Unicorn comparisons. Native
witnesses compare the C profile directly against the original instructions.
The model consequently claims the pinned normal/386 profile, not universal
undefined-flag behavior across x86 implementations. The full caller
differential fixtures themselves must pass all 14 words without exceptions.

The synthetic C regression exercises lower-pair rejection, successful
replacement with and without a selected sidecar, atomic invalid-input
guards, and budget exhaustion. CMake/CTest and an ASan/UBSan build pass.
Independent source inspection maps 109/109 caller starts and five actual
call sites; independent harness inspection checks capture ordering,
relocation, region boundaries and declared coordinate reconstruction.

Reproduce from the repository root with a Python environment containing
Capstone and Unicorn and owner-provided media under ignored resources/DOS:

    python tools/overlay/check_checker_builds.py OVL_009E_115C RES_00BA_0BB8 --require-current-main
    python tools/overlay/u9e_character_pair_115c_audit.py --output resources/u9e-115c-audit.json
    python tools/overlay/u9e_character_pair_115c_native.py --output-dir resources/u9e-115c-native

The native command additionally needs GNU binutils (as, ld, nm) and DOSBox-X. The compile-only
checker builds and binds the same C source list used for parity and needs
no game media.

## Parity assessment for independent review

| Field | Author assessment |
| --- | --- |
| target_function | OVL_009E_115C with complete RES_00BA_0BB8 closure |
| source_candidates | overlay_u9e_character_pair_115c_cpu.c; resident_00ba_character_0bb8_cpu.c; resident_00ba_character_helpers_cpu.c |
| signals | Pinned original spans; fixup-backed callers and effect-slot binding; member/list layout; source-ordered control flow and memory effects; actual callee frames; unmasked differential and native witnesses |
| score | Strong measured behavioral agreement within the declared execution profile; no numerical confidence inferred |
| verdict | match within the documented immutable-code, normal/386 profile |
| review_notes | Synthetic entry states; no natural game route; native RAM subset; declared checkpoint interception; raw-Unicorn boundary disagreements listed above; independent acceptance pending |

The readiness manifest's full status means the complete resident callee is
implemented for composition under this contract. It assigns neither game
identity nor strict credit. This PR does not update either credit ledger.

Measured report SHA-256 values (private captures remain under resources/):

- oracle-final.json: 7cd5537c1b07569d600e6008d8758f8f83441d51d860572043de7fa86eb6ed98
- oracle-native/report.json: ab39eea1dae20b8db83b95f74289fe1d2b0c53c2fe99e6e579fb13d83a6a3237
- helpers-oracle-review.json: 5df1065c81bfaa4dc3b7d3b71826e718cf543731c44dd6ef276875aa8aa71b0d

## Declared resident helper targets

RES_00BA_1168 and RES_00BA_13AB are separately declared full callees in
the readiness manifest and registered with this same checker. This exposes
already implemented bodies for independent review; it adds no translation,
new test run claim, or author-assigned credit.

| Target | Original span | Bytes / starts | Existing measured branch outcomes |
| --- | --- | ---: | ---: |
| RES_00BA_1168 | [1168,11EA) | 130 / 54 | 22 feasible of 24 syntactic; the two impossible directions are proved above |
| RES_00BA_13AB | [13AB,148B) | 224 / 89 | 36 of 36 |

**Source provenance (verified during the #1162 free-model screening).**
`RES_00BA_13AB` is a loader-expanded **START.EXE** body at file
`[0x2027,0x2107)` = `[8231,8455)`, 224 bytes, SHA-256 `22732f3c…4b7bbe`; it is
**not present in GAME.OVR** (the 224-byte body and even its first 32 bytes do
not occur there). Grepping GAME.OVR finds a **12-byte prologue decoy** at
`0x2BEBC` that matches the body up to and including `ff 76 06` and then
diverges at byte 12 (`b0 19` where the body has `0e e8`). The same applies to
the rest of this resident closure: its source is the loader-expanded START.EXE
image, not the overlay file.

The published 3,441-case audit includes all starts and feasible outcomes of
both bodies through U9E-origin execution. It also includes 256 standalone
+1168 inputs and 512 standalone +13AB inputs. These are subsets of the
existing totals, not additional comparisons. The existing 33 native
captures include two completed wrapped-SP entries for each helper and
three explicitly intercepted MUL checkpoints inside +13AB. The same native
RAM subset, coordinate provenance, raw-Unicorn disagreements and
normal/386 restrictions apply. No separate full-1MiB native claim is made.

Body SHA-256: +1168
4c5c49b63f5a382b3de7df784840f406cd807921aeeffbb88948483067ac49a4;
+13AB
22732f3cfe79dc6feb87722de05c3da242b1d3c7f984db735f8c3d38ef3b7bbe.
The unchanged evidence reports above pin the already measured execution.
VP i19181 requested these declarations; the independent reviewer retains
all acceptance and credit decisions.

## Identity of the nested near-only bodies (par-a, 2026-10-21)

Eight bodies of this closure are near-only resident bodies: `+0000`, `+01FB`, `+0282`, `+03A0`, `+10E6`, `+11EA`, `+1282` and `+1325`. Their two-factor identity verdicts (all PASS) are in [NEAR_ONLY_GAME_SEGMENT_IDENTITY.md](NEAR_ONLY_GAME_SEGMENT_IDENTITY.md): direct near sites inside strict-credited `+0BB8`/`+13AB`, and the character object passed as an argument and used.
