# Synthetic COMPARE numeric flag comparisons

The bounded U25:18FF continuation performs six unsigned word comparisons, rereading SS:BP+8 and SS:BP+6 before each one. It conditionally writes 1 to the corresponding DS:6F78..6F7D byte for equal, not equal, below, above, below-or-equal and above-or-equal, respectively. False conditions leave existing bytes unchanged. Each read and write uses the shared bus, so flag/frame aliases can affect later comparisons. The final CMP flags and AX remain live; execution stops at 194D before the helper return.

The direct entry requires ordinary CS, DS and SS, SP=BP with BP at most FFF7, 1 MiB RAM and attached EGA. The composition inherits [FillChar return](U25_COMPARE_FILL_RETURN_SHARED_BUS.md) frame, alias, immutable-code and nested-session profiles, and passes earlier stops through. Tests cover equal, less, greater, unsigned high-bit cases and final flags, an operand/flag alias that changes later results, invalid-frame atomicity, and synthetic integration. The operation order reuses [numeric helper evidence](OVL_0025_18EB_CPU.md).

Prior SAVEs and operands are synthetic. Native reach, the helper epilogue and return, complete COMPARE, readiness and credit remain unverified. The flat-RAM numeric helper is not composed.
