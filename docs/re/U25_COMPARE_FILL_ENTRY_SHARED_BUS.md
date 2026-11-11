# Synthetic COMPARE numeric FillChar entry

The bounded U25:18EB continuation saves BP, sets DI to 6F78, pushes DS and DI, pushes length 6 and fill byte 0, and checks the relocated far call at 18FA to resident 16A8. It pushes the actual return frame and stops before FillChar. Flags remain unchanged; malformed calls retain earlier effects. Stack accesses use the shared bus in source order.

The direct entry requires ordinary CS, DS and SS, SP in [000E, FFF8], exactly 1 MiB RAM and attached EGA; the reached resident segment must be ordinary. The composition inherits [numeric helper entry](U1C_COMPARE_NUMERIC_ENTRY_SHARED_BUS.md) frame, alias, immutable-code and nested-session profiles and passes earlier outcomes through. Synthetic integration and standalone tests cover the argument frame, flags, alias, malformed opcode/offset and invalid-frame atomicity. The sequence and call target reuse [numeric compare evidence](OVL_0025_18EB_CPU.md).

Prior SAVEs and operands are synthetic. Native reach, FillChar body, numeric comparisons, full helper return, complete COMPARE, readiness and credit remain unverified. The flat-RAM numeric helper and full load-word provider are not composed.
