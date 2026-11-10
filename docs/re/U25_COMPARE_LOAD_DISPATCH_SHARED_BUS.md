# Synthetic COMPARE load-word class dispatch

The bounded continuation executes U25:0E19 class-byte and zero-flag stores, rereads the live class byte, and compares it against 0 through 4 in source order. It stops before class bodies at 0E27, 0E41, 0E70, 0E8A or 0EA7; unmatched values stop at 0EA4. Final comparison flags and other registers remain live. The direct entry requires ordinary CS, DS and SS, BP at least 6, SP=BP-6, 1 MiB RAM and attached EGA. All local accesses use the shared bus.

The composed wrapper inherits [classifier-return](U25_COMPARE_CLASS_RETURN_SHARED_BUS.md) frame, alias, immutable-code and nested-session profiles and passes earlier stops through. The synthetic integration exercises class four for address 2208. Standalone checks cover all 256 class bytes and independently calculated flags, stack/data aliasing and invalid-frame atomicity. The dispatch instruction boundaries were decoded read-only from owner media and agree with [load-word evidence](U25_LOAD_WORD_0E0C.md); no game bytes are included.

The preceding SAVEs and operands are synthetic. No class body, native reach, full shared-bus load-word return, completed COMPARE, readiness or credit is claimed. The flat-RAM full load-word provider is not composed.
