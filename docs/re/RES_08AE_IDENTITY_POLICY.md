# 08AE dependency band: policy closure

Disposition: VP i19467, 2026-10-20; evidence baseline **a32aa524**.
This receipt closes the 53 candidate rows that list `RES_00BA_16EC` among their
callees, of which 21 are in unit 0x38 (57 candidates there), recorded in the
[queue](STRICT_CANDIDATE_QUEUE.md), whose measured three-hop floor is 1,932 B.
(Wording corrected 2026-10-20: the original text called these rows "the 53-row 0x38
band"; the count is across units. The 1,932 B floor was not re-derived.)

08AE is documented as the timer/audio driver; its internals are audio-driver
and service code, executing normally, not game logic.
No current route credits them: no credited caller includes them, all 28 overlay
references to the segment are the known 00DC/01A5 calls, the observed caller is
uncredited, and saved gameplay reaching three of them supplies provenance of
**use**, not of authorship.
The band is closed by the existing policy that excludes device-facing internals
from game credit—not by absent evidence, and not by anything a future capture
would likely change.

These sentences record the VP's scope decision for these audio-driver/service
internals. **Driver role is established; authorship remains unresolved.**
They do not establish an external-library source match, change classifications,
or retroactively exclude other credited device-facing routines.

## Evidence and verdict

“Unreachable” means unavailable as a **game credit under this policy**, not
impossible execution. These ordinary helpers do not become environment endpoints
eligible for declared_boundary merely by belonging to a driver.

| Body | Evidence class | Artifact carrying it | Verdict |
| --- | --- | --- | --- |
| 181A, 22 B | Stores the argument byte to local 08AE:020D; output routines 1477/14EC read it at 14C0/1519 to gate audio output. Calls from unresolved 00DC at 010E/011D. | [Translated leaf](RES_08AE_BYTE_181A_CPU.md); [driver census](FUNCTION_CEILING_CENSUS.md#segment-0x8ae-range-no-functions). | Unreachable as current game credit; translated/uncredited in #1185. Not declarable. |
| 1742, 108 B | Table 08AE:055F + 8×index supplies four stream pointers to records at 022D; timer path 1477 and interpreter 160B consume them. Saved one-Up reaches arguments 9 and 0. | [Writer](RESIDENT_8AE_1742.md), [local writers](RESIDENT_8AE_LEAVES.md), [native dispatch](SLOT_A_ONE_UP_NATIVE_RESIDENT_DISPATCH.md), driver census. | Unreachable as current game credit; driver producer. Not declarable. |
| 17AE, 108 B | Parallel table at 08AE:0607 and records at 03C7; timer output path 14EC feeds port C0. Saved one-Up reaches argument 0. | Local writers, native dispatch, driver census. | Unreachable as current game credit; driver producer. Not declarable. |
| 006C, 112 B | Waits on repeated calls to 08AE:0000, which gets DOS time through 0A32:00A2 and computes hundredths; no game-record/table input. Saved one-Up reaches argument 10. | Native dispatch; [time-prefix evidence](SLOT_A_SECOND_KEY_RESIDENT_001F_CALLER_PREFIX.md); private expanded-byte trace. | Unreachable as current game credit; ordinary timing service, not a direct environment endpoint. |

The census documents handler 13EA, output paths 1477/14EC, interpreter 160B,
two 21×8 song tables, and channel, wave, envelope and stream data.
None of these consumer IDs is strict-credited. Its six-entry opcode dispatch
table targets interior labels in 160B, not these leaves; it supplies no
credited game-owned dispatch table. The census explicitly measures callable
bodies, **not authorship**.

## Caller credit and capture limits

The [metadata receipt](08ae_identity_scope_receipt.json) contains all 28
overlay references, media hashes, strict membership and capture hashes.

- 19 fixup-backed calls target 00DC; nine target 01A5. There are zero direct
  overlay calls to each leaf and no overlay data/procedure pointer constant
  referencing segment 08AE.
- Ledger-contained 00DC callers are U1C:2CCF/32F8; U5E:0712/1406/26B0;
  U68:073B/0989/0D6D; U86:0C20/1658; U3C6:0E11. All are uncredited.
  The additional 01A5 caller U36:002D is also uncredited. These ledger ranges
  identify containing rows; they are not new executable-span claims.
- **U68 has eight credited routines, but observed caller U68:073B is
  uncredited.** Credit attaches to routines, not the whole overlay unit.
- Global mode DS:5372 and active WORD DS:5374 are distinct from the local
  resident segment holding 020D and the tables. The saved run has global
  DS=14A0 and resident segment 10D2, mode 1 and active 100. Controlling a driver
  mode does not establish ownership of its implementation.
- Six existing 640 KiB snapshots match their published hashes; their two-table
  region matches expanded media. The argument-10 path reaches 1742(9),
  006C(10), 17AE(0), then 1742(0); **181A is bypassed**.
  No new capture or full callee-output/sound-waveform comparison is claimed.

Reaching a driver from a game action establishes use, not game-source identity.
A game-sound correspondence would identify an asset/use, not automatically
establish the generic producer as game logic. No attribution-bearing game string
was found in inspected resident data; its decoded message is a generic runtime
error. Raw table-window matching against other supplied top-level DOS files
found no match. Packed/transformed relationships are not excluded, and this
negative search is not the basis for the policy decision.

## Port check and authorship limit

A fresh direct-CFG scan from all **244 strict roots** found no direct OUT to
42/61/C0. It found three direct OUTs, all in credited RES_0709_01DF:
03AD immediately follows a DX load of 03C4; 03D5 and 0470 follow loads of 03C5.
These are EGA writes. **Calls were not expanded**; this is a direct-body result,
not a claim about every transitive runtime operation. No decode or indirect-jump
gaps were encountered.

The credited glyph body is itself device-facing, which prevents inferring
external-library authorship merely from port operations. An attributable source,
object or linker-map match, or an independently identified library counterpart,
could answer authorship; none is supplied here. Behavior alone permits both
in-house-driver and linked-library explanations.

## Reopening and cost

Reopening is an **owner-level policy decision**. The VP names the consistency
cost: refused [RES_0464_2A0D](RESIDENT_PALETTE_2A0D_GATE.md) and the
[0x7F pointer-helper pair](STRICT_CANDIDATE_QUEUE.md) would come back into scope.
This receipt proposes neither reopening nor a strict/manifest row.

Cost class: **an hour of reading**, the bounded scoping budget, completed within
that budget using existing captures. A new day of capture would establish more
use, not override the policy. Searching for source/library provenance has no
fixed delivery estimate and would not itself change the game-logic policy.

## Preserved inputs and reproduction

The public JSON contains small address/count/hash metadata only. Original media,
derived disassembly and raw captures remain ignored. The complete private
receipt, JSONs and three analysis scripts are preserved outside /tmp under
resources/08ae-identity-scope-20261020/ in the author workspace. The public JSON
records hashes of the original private receipts.

From a worktree at the baseline, with owner media at resources/DOS/POOLRAD and
the audit Python environment (Capstone 5.0.7), the preserved scripts regenerate
the fixup/credit, relocation/data and direct-body port receipts:

    python3 resources/08ae-identity-scope-20261020/08ae_identity_scope.py
    python3 resources/08ae-identity-scope-20261020/08ae_data_scope.py
    python3 resources/08ae-identity-scope-20261020/08ae_strict_ports.py

They write only ignored resources. Existing capture hashes can be rechecked in
resources/oracle-runs/slot-a-resident-live-005/; that directory and the native
dispatch note name the same six captures listed in the JSON.
Regeneration needs those private owner inputs; JSON inspection does not.

Dunking Sheep recall reused session 281c8700-5acc-438b-b93d-c061e28437eb,
chunk 20188, for the native-route lead, and session
a3ad1536-abb2-4fb8-af2c-1362b8656678, chunk 36043, for the census lead.
Media, credit membership and existing capture hashes were checked directly.
This documentation changes no C, classification, manifest, credit or accounting.
