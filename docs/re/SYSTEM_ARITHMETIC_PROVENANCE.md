# SYSTEM arithmetic native executable provenance (t108)

The existing `system_longmul_0a5b_audit.py --native` and
`system_longdiv_0a76_audit.py --native` commands now resolve their existing
`dosbox-x` PATH choice through `native_emulator_provenance.describe_dosbox`.
They launch the returned absolute path and include its version, SHA256,
version exit status and `pinned_build_used` in `native.dosbox_x`.
The helper truthfully reports false for an unattested installed executable.
Incomplete provenance refuses before emulator launch.

The command arguments, environment, timeout, deterministic cases, generated
COM/config, C source, CPU14/32-byte stack checks and expected-result comparisons
are unchanged. This records executable identity; it adds no native case,
mask, runtime behavior, recovered-function credit or emulator pin.
The synthetic launch-boundary tests stop before executing an emulator.

The touched legacy audits also declare their existing resident identities
`RES_0AF8_0A5B` and `RES_0AF8_0A76` for the checker/document pairing gate.
These are the existing linked SYSTEM runtime bodies; no credit table or
accounting pin is changed. The declarations add no new game-body claim.
