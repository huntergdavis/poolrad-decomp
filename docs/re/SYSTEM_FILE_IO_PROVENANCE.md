# SYSTEM Close and FileCheck emulator provenance (t108)

The existing Close `system_close_1491_cpu_audit.py --native` and FileCheck
`system_filecheck_14b6_cpu_audit.py --native` audits now use the accepted
`native_emulator_provenance.describe_dosbox` helper before launching DOSBox-X.
Their original PATH choice is resolved once and the returned absolute path is
launched. `native.dosbox_x` records that path's version, SHA256, version exit
status and truthful `pinned_build_used=false`; its pinned-source field names
an intended source revision, not an attested build.

Previously each audit launched a bare PATH name and separately ran `--version`
and `which` after execution. The helper replaces those independent post-run
probes. It does not eliminate replacement-between-hash-and-launch races.
Incomplete provenance refuses before emulator launch.

Command arguments, environment, timeout, generated COM/config, 48 deterministic
cases per audit, C bindings, service substitutions and comparison windows are
unchanged. Close retains its declared DOS service hook and eight excluded
interrupt-frame bytes. Neither audit compares all physical RAM. No new mask,
case, credit table or accounting pin is introduced. Existing CHECKER_ROWS and
media-free compile-only behavior are preserved.

Focused synthetic launch tests stop before executing an emulator and check
command identity, incomplete-provenance refusal and real C compile-only binding
without media or emulator access. Native validation uses the existing cases;
new game behavior, fixture adequacy and a pinned emulator are separate claims.
