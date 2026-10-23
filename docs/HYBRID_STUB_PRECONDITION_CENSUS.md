# Census: models that call another overlay unit without reading its stub (VP i21329)

## Why

The hybrid executor's shadow check (batch 5, [HYBRID_GLUE_BATCH5.md](HYBRID_GLUE_BATCH5.md)) caught `OVL_006A_0214`
`C_DIFFERS` on its second arrival.

- **The first differing byte** was `PSP:002E`, where DOS saves the caller's SP on each `INT 21h`.
- **The cause** is in 0214's callee `OVL_006A_019C`. Its model runs unit 0x86's `+323D` model "through its loaded stub",
  and never reads the stub at `0096:003E`.
- **In the live game** the unit was not resident on that arrival. The original went through the overlay manager, which
  loaded the unit with DOS reads, and the C skipped that.
- **Why no earlier test saw it:** end-state differentials always run with the unit present. The first live arrival was
  equal for the same reason.

This is a class. Any model that runs a callee in another overlay unit stands in for a far call through that unit's stub,
and is valid only while the unit is resident, unless the model reads the stub.

## Method

`tools/hybrid/stub_census.py` covers every `src/recovered/*.c` whose entry has the raw-CPU shape (`uint8_t *ram, size_t,
CPU *`) and that calls a `pr_uXX_*` model of another overlay unit. It sorts them into three classes:

- **(a) Reads the stub:** the source tests for `EAh`, the far JMP of a loaded stub.
- **(b) Declares the precondition:** no `EAh` test, but the source or its header says the unit or stub is loaded or
  resident.
- **(c) Neither.**

The detection is lexical, so **every line below is a lead for a reviewer, not a verdict**. In particular, a file in (a)
may test `EAh` for another reason. Semantic (state-struct) models are out of scope, because overlay residency is below
their contract level.

## Result (main `435c6f65`)

| Class | Models | With a credited ID |
|---|---|---|
| (a) reads the stub | 32 | 30 |
| (b) declares "loaded", no runtime check | 6 | 6 |
| (c) neither | 4 | 3 |

**(b) and (c), by body:**

| Class | Body | Source | Calls unit |
|---|---|---|---|
| b | `OVL_001C_0B52` | `overlay_u1c_if_0b52.c` (U25's CS is passed as an argument) | 0x25 |
| b | `OVL_006A_019C` | `overlay_u6a_link_high_clear_019c.c` (**seen live**) | 0x86 |
| b | `OVL_006A_0000` | `overlay_u6a_record_weight_0000.c` | 0x86 |
| b | `OVL_005E_263D` | `overlay_u5e_direction_263d.c` | 0x3C2 |
| b | `OVL_005E_1BF8` | `overlay_u5e_neighbor_1bf8.c` | 0x3C6 |
| b | `OVL_007F_1010` | `overlay_u7f_caller_1010.c` (`dice_cs` is passed as an argument) | 0x9E |
| c | `OVL_0038_04BA` | `overlay_u38_pair_04ba.c` | 0x9E |
| c | `OVL_03C2_0909` | `overlay_u3c2_caller_0909.c` | 0x3C6 |
| c | `OVL_005E_32E6` | `overlay_u5e_caller_32e6.c` | 0x3C2, 0x3C6 |
| c | `OVL_009E_0160` (not credited) | `overlay_u9e_sidecar_0160_cpu.c` | 0x3C2, 0x3C6 |

## What it means

- **Nothing here shows a model's arithmetic is wrong.** These nine credited bodies are valid only while the unit they
  call is resident, and their code does not check it.
- **Whether a credit's evidence must state the precondition is the review lane's decision.** The 019C evidence went to
  par-e (i21375).
- **The hybrid's handling.** Today it guards `019C`, `0214` and `0B52`: their C runs only when the stub is a far JMP to
  the expected entry. Otherwise the original runs, including its overlay load. The other bodies are not glued; each will
  get the same guard when it is.
