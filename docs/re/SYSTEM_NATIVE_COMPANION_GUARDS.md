# SYSTEM text-write and Reset native companion entrypoints

`system_text_write_cpu_native.py` and `system_reset_1410_native.py` contain
importable `native()` implementations. Running either file directly previously
returned success without executing cases. Each now refuses before optional
imports, with exit 1 and the real owning audit command:

- `tools/overlay/system_text_write_cpu_audit.py --native`
- `tools/overlay/system_reset_1410_cpu_audit.py --native`

Run legitimate audits through `poolrad-heavy.sh`. The refusal is covered both
in the normal Python environment and with `-I -S`, where optional audit imports
are unavailable. Reset needs an explicit audit-name override in the regression
because its owner includes `_cpu` but its companion filename does not.

The guard is the only executable addition to either companion. Imported native
functions, case construction, C models, assembly, capture windows and masks are
unchanged. This is a bounded entrypoint correction, not emulator pinning,
fixture adequacy, whole-game parity or new function credit. Existing declared
substitutions in the native harnesses still apply.
