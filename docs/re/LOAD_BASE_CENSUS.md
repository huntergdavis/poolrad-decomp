# The load-base census: constant segments in far calls (VP i20604, 2026-10-21)

**Three fixture instruments, one question.** The [dirty-register probe](DIRTY_REGISTER_SWEEP.md), the operand-equality probe
([OPERAND_EQUALITY_SWEEP.md](OPERAND_EQUALITY_SWEEP.md)) and this census each answer *did the fixture ever take the value the
instruction depends on*: a register's low byte, the equal operand of a compare, the loader's relocation of a segment word.
Branch accounting, start coverage and a passing differential cannot answer it, because every fixture can agree with the model
on a value neither of them varies.

**Thesis: the census proves nothing about a fixed model by itself; each fix carries the spy.** The census finds candidates;
the catcher proves the defect; the fix carries the proof.

## The defect class

A far call in the original is `9A offset segment`, and the loader relocates the segment word. A model that sets `CS := 00BAh`
(or any literal, or a macro defined as one) is right only at the unrelocated base. It is **latent in every differential**: the
audit runs at the base where the constant equals the word, and the callee's own model never looks at the CS it is entered
with, so even a differential at another base would see the caller do the right thing and the callee ignore it.

## What found it, and what did not

**GREP IS NOT THE CENSUS.** par-e's sweep, written as a regex over `0x0?BA`-shaped literals, missed `0918` because the
regex allowed one leading zero and `overlay_u38_sidecar_flag_0918.c:118` reads `cpu->cs = 0x00BAu`. The confirmed count was
therefore wrong in circulation: **four models, seven sites** (0DE3/0E0E 1, 1010 3, 0918 1, 275B 2), and the census below later
added `0FDE` (`cs = 0x00B0u`), making **five models and eight sites**.
`tools/overlay/load_base_census.py` is a different thing from that regex: it reads **every** assignment to a `cs` field in
`src/recovered/*.c`, whatever the width of the literal, and also resolves a macro defined as a hex literal. It fails closed on
any site not on a short allowlist. Run on the tree before the fixes it finds exactly: `overlay_0077_275b_cpu.c` (2),
`overlay_u38_sidecar_flag_0918.c` (1), `overlay_u7f_caller_1010.c` (3), `overlay_u7f_roll_call_0fde.c` (1), plus the three
allowed native-capture sites below (and `0E0E`, which par-c had already fixed in #1376).

## Two mechanisms, because a moved base alone cannot see a callee that ignores its CS

1. **The caller reads the live word.** Fixed by taking the segment from the word in the caller's code. Where the audit
   executes the callee (0E0E at a relocated SYSTEM segment) the moved base is observable through the aliasing guard.
2. **The callee is entered at that word (an entry-CS spy).** Where the callee's model ignores its CS (every leaf here), the
   only catcher is a spy that records the CS it is entered with. Each fix carries one, and each fails on the old model and
   passes on the new:

| Model | Sites | PR | Catcher (fails on the old, passes on the new) |
| --- | ---: | --- | --- |
| `OVL_0077_275B` | 2 | #1373 | the two resident models replaced by spies recording entry CS; words 00BA, 1234/5678, 0001/FFF0, 10BA |
| `OVL_0038_0918` | 1 | #1374 | the lookup callback asserts it is entered at the word; five word values (the old test asserted the constant) |
| `OVL_007F_0FDE` | 1 | #1375 | the roll callback asserts the word; five values (the old test asserted the constant) |
| `OVL_007F_1010` | 3 | #1380 | FillChar wrapped at link time (`-Wl,--wrap`), three sites x three word sets x 256 types, all sites reached (4,610 checks) |
| 0DE3 / 0E0E | 1 | #1376 (par-c) | the aliasing guard follows the live word |

A generic "rerun every audit at base B" harness was **not built**: those audits place their callees at constants of their own
choosing, almost none of them the true loaded segments, and where a callee is a C model it never reads its code or its CS, so
the rerun would pass on the old models too. The per-model spy is what observes the defect; the census finds where to put one.

## Results, in the three lists

- **FAILS (fixed, each with a fails-then-passes pair):** 275B, 0918, 0FDE, 1010 (this assignment), 0DE3/0E0E (par-c).
- **PASSES (70 sites):** assignments whose right side was read from RAM a few lines earlier (the heuristic `--classify`
  split: LIVE 70 sites in 61 files). That the segment is *read* is shown by the scan, not that the callee is entered at it for
  every model; the spies above prove it only for the five fixed models.
- **NOT APPLICABLE, with the reason:**
  - `resident_464_2632_run014.c:122,157` and `slot_a_directory_fullstate.c:136` (`0x131C`): models of **one captured native
    run**; `0x131C` is that run's loaded SYSTEM segment (`0AF8 + 0824`), a value pinned to one *loaded* base rather than an
    unrelocated one, so a constant is the contract. This is the reverse case and is allowlisted by name with that reason.
  - `overlay_u3b9_entry7_0708_cpu.c`, `overlay_u3c6_0105_cpu.c` and `resident_font_loader_017c.c` compare or add
    **differences** of live words and a `load_segment` bias: base-independent by construction, the reference pattern.
- **PARAM (27 sites in 19 files) and OTHER (37 sites in 5 files, mostly `overlay_u25_string_cores.c`'s `system_cs`):**
  heuristic: the segment is a parameter the host supplies, so the model ignores the code word. Right whenever the host passes
  what the loader put there; the load-base class **one step removed**, not counted as defects and not fixed here. A host that
  derives the parameter from the live word is the intended pattern.

## The fixture symptom to expect

A synthetic fixture that builds a fake loaded caller **without the LCALL bytes** now fails loudly when the fixed model reads
the word (it reads fill). That is the symptom this class exists to produce, not a regression: the fix is to install the
`9A offset segment` bytes (the tests of 0918, 0FDE and 1010 each gained their call words; par-c's fixes added them for 0DE3's
composers). The census is `tools/overlay/test_load_base_census.py`: it asserts the committed result is exactly the three
allowed native-capture sites, so a new constant-CS assignment fails the test.

## Date correction carried by this change

Several documents of this night were dated `2026-10-22`; the date is **2026-10-21**. The 275B, 0918, 0FDE and 1010 fix pull
requests were corrected before merge. Two documents were **already merged** with the wrong date and are corrected here:
`DIRTY_REGISTER_SWEEP.md` (its title line) and `RES_0709_SPACE_05EB_CPU.md` (the checker-registration heading). A date in a
corpus is a claim about when evidence existed; the correction is stated here rather than made silently.
