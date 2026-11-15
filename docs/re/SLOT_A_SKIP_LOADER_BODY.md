# Skipped SAVE numeric loader

The existing `U25_LOADER_0000.md` and `U25_SKIP_1EA6.md` contracts describe
the raw loader and the skipped SAVE's two-operand call. The retained script
audit identifies the following SAVE; this shared-state fixture supplies a
numeric immediate and address synthetically.

`pr_slot_a_skip_loader_body` accepts only tag 00 followed by tag 02, validates
the caller-authenticated loader body and count frame, excludes the declared
stack/DS/source aliases, and invokes the existing raw loader with two operand
iterations. It exposes the loader's actual status and CPU/RAM. The caller
authenticates the SYSTEM binding, keeps code immutable and host objects
disjoint. Unsupported profiles refuse before mutation.

The synthetic chain checks the numeric table bytes and script PC after the
loader, plus unsupported-tag refusal. It does not execute the remaining skip
or IF returns, establish same-invocation provenance, complete the Up route, or
prove native 345B parity.
