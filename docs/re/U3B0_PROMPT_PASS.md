# Unit 3B0 single-pass prompt input composition

`pr_u3b0_prompt_pass` composes the merged poll, character, selection,
mapped-key, text-search and return diagnostics on one caller-owned CPU/RAM
and EGA bus. Starting at a supplied live `+0566` frame, one pass may reach the
actual restricted `RETF 16h` target, the `+048E` loop boundary, or a named
nested/profile stop. It does not repeat the loop or manufacture an entry2
`provided` result. The FIFO can be supplied as the poll's existing INT16
service, but this API does not create an OS keyboard source.

The live entry is **after** the prompt's earlier setup, output and drawing;
prior caller and frame state, prompt string/list construction, window/device
state and the reached `+048B` drawer effects must already be supplied by the
host. The `+048E..+0565` waiting path may have timer/clock effects on other
inputs. The bounded `SLOT_A_POSTDRAWER_WAIT_PAGE.md` capture observes one
conditional no-draw route, with no retained same-run CPU transcript; it is
not proof that this pipeline established prior effects. Source identity,
loaded relocations, stable code, services, sets, tables, stack and the expected
return target are caller-owned and their provenance is not proved by a flag.

The pipeline composes existing flat-RAM poll/character/return models with
bus-backed selection/mapped/text models. It requires ordinary non-EGA CS, DS,
SS and declared CRT, key-wrapper and SYSTEM segment windows, and an ordinary
flag-pointer segment at entry. The caller must ensure the live flag-pointer
storage remains unchanged until the poll's flag write, and all flat-RAM
component/nested-service accesses and writes remain outside the EGA aperture;
code, set and callback restrictions of each component also apply. These are
caller obligations, not dynamically verified whole-execution claims. Before
the flat-RAM epilogue, a guard checks its frame, both window-clear bytes and
flag byte for EGA aperture, and conservatively refuses if the window clear
would overwrite the flag-pointer storage before its live reread. `PROFILE`
reports the entry/return-guard stage and a segment, frame, aperture or alias
reason; a return-guard refusal preserves effects already performed by the
poll or later stages. The profile is deliberately narrower than the original.

A no-key poll follows the source through `+06CF` and may stop at `+048E`; it
is not a resumable yield. An empty `AH=00` service decline stays a terminal
nested CRT stop after prior effects. Text loop budgets stop before the specified
taken jump, retaining state; nested failures, code checks and return-target
mismatches are reported with their component diagnostics and no rollback.
`RETURN` means only that the source-bounded epilogue reached the caller-supplied
CS:IP from the supplied frame. It does not prove earlier prompt behavior,
provide an entry2 hook, or establish full prompt, native/gameplay/visual parity
or strict credit.

Synthetic tests use the actual FIFO, CRT, 0899, SYSTEM and EGA drawer models:
extended Up, Escape, text match/redraw and comma/redraw, whole CPU/RAM/EGA
no-event loop, terminal nested read refusal, bad call after prior effects,
text budget, return mismatch, entry aperture rejection, post-input aperture
rejection and return-pointer alias rejection. They validate the declared
composition, not natural reach or native equality. Component source and
identity references are in the adjacent `U3B0_*_DIAGNOSTIC.md` documents.
