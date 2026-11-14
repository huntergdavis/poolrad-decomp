# Slot-A key dependency to projected first EXIT

`pr_slot_a_key_exit_handoff` consumes the dependency emitted by
[the key/script join](SLOT_A_KEY_SCRIPT_JOIN.md) and invokes the existing
[first-EXIT runner](SLOT_A_FIRST_EXIT_9974.md) on a caller-supplied live
`+3171` entry image. It checks the dependency tuple `(001C,3458,3171,345B,9914)`,
the live `DS:49E3` entry word, and the stack's return IP, caller CS, and
argument before running. The caller must establish that the dependency and
image are from the same invocation; these field checks do not prove provenance.
The objects must be nonoverlapping and the image must outlive the returned ECL
state.

The input RAM is read-only. A successful result is a **projected** script
return to `345B`, with ECL state and EXIT environment in host objects; it does
not write them into guest RAM or produce a raw CPU/stack return. Binding errors
and non-OK runner or handler outcomes remain explicit, including an unmodeled
opcode, nonempty GOSUB list, and missing EXIT hook. Earlier runner effects can
remain in the projected state on a blocked return. No hook is fabricated.

## Evidence and checks

The key join's return/gate captures are same-invocation, but its retained script
entry is from a separate invocation, as the key-join audit states. The existing
first-EXIT audit uses that script-entry image (655360 bytes, SHA-256
`f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558`). Its
frame is `345B/3582/9914` and `DS:49E3=9914`. A hash-pinned check supplies the
dependency tuple explicitly to the new handoff and verifies ten modeled steps,
projected PC `9975`, and unchanged input RAM. It does **not** claim that the
separate key capture led to that image in the same invocation.

Synthetic checks feed an actual dependency returned by the key join into a
coherent caller-owned fixture. They cover projected EXIT, mismatched entry and
frame, nonempty call list, unmodeled opcode, missing EXIT hook, and preservation
of outputs on pre-run refusal. The existing key-join regression also passes.
Reproduce without private inputs with
`bash tests/replay/check_slot_a_key_exit_handoff.sh`; pass an ignored script-entry
RAM path as its sole argument to run the additional hash-pinned check. The
`poolrad_slot_a_key_exit_handoff` CMake target builds the reusable library.
Original captures remain under ignored `resources/`.

## Remaining connection

The saved-game joined route still stops at `BLOCKED_KEY_DISPATCH`; it does not
produce a same-invocation script-entry image for this handoff. The projected
EXIT state is not the C-produced raw `345B` frame needed to continue through
entry 1 and the resident path to `2E60:0747`. A future integration must provide
that frame and memory as results of the intervening C execution, preserving all
service failures; a later capture cannot be substituted as its output. The
separate conditional DOS seek at `028F` also remains blocked on this request's
post-service CPU, ordered RAM journal, and handle binding. This change adds no
native same-invocation, complete post-Up frame, or full-Up parity claim.
