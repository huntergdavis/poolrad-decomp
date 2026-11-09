# SAVE destination and tag branch

`pr_u1c_save_tag_branch` executes SAVE `+02AD..+02B5`: write AX to SS:BP-4, read the live tag at DS:7006, compare it with `80`, and stop at numeric continuation `+02B7` or string continuation `+02CF`. The shared-bus write precedes the tag read, so an alias can change the branch. The comparison preserves its arithmetic flags and the other registers are unchanged.

`pr_save_tag_start` and `pr_save_tag_resume` extend the [checked operand-word return](U25_WORD_RETURN_SHARED_BUS.md) to those distinct continuations. Earlier loader-string, budget, prompt and other stops remain distinct; inherited suspension and frame checks still apply. Neither continuation helper nor later SAVE effects are executed.

The [pinned SAVE audit](CANDIDATE_SCOUT_DS41B_BATCH1.md) identifies GAME.OVR SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` and SAVE `[0292,02E0)` SHA-256 `7f66f8be811fe835d3ed3ab0fc0772f57e10ab39b5b5beda8aa2542262f92ebb`. No original bytes are embedded.

Standalone entry requires IP `02AD`, BP at least 4, exactly 1 MiB RAM, attached EGA, and ordinary CS/DS/SS windows below `9000` or at least `B000`. Caller owns immutable code and valid reached frames; the composition inherits prior bounds and profiles. These are bounded assumptions rather than a general alias or hardware proof.

Synthetic tests cover the idle-to-Up numeric route, the string branch, flags, an alias in which the local write changes the tag, and invalid-entry atomicity. The wrapper and fixture pass strict-warning ASan/UBSan with the provider archive uninstrumented. This composition has no native differential; string execution, later SAVE effects, movement, visual parity, full gameplay and coverage credit remain unverified.
