# Synthetic COMPARE numeric helper entry

The bounded U1C:0191 continuation stores AX at SS:BP-4, rereads and pushes the first operand at BP-2 and the second at BP-4 in order, and checks the far call at 019A through the loaded 0070 jump stub. It pushes the actual return frame and stops at U25:18EB before executing the numeric comparison helper. Flags remain unchanged. Malformed call or stub checks preserve earlier source effects; stack reads and writes use the shared bus.

The direct entry requires ordinary CS, DS and SS, BP at least 020C, SP=BP-0204, 1 MiB RAM and attached EGA. The wrapper inherits [second-slot return](U25_COMPARE_SECOND_VALUE_RETURN_SHARED_BUS.md) frame, alias, immutable-code and nested-session profiles and passes earlier stops through. Synthetic integration and standalone tests cover argument order, flags, aliasing, malformed call and stub, and invalid-frame atomicity. The instructions were decoded read-only from ignored owner media and agree with candidate [COMPARE evidence](U1C_COMPARE_0126_CPU.md).

Prior SAVEs and operands are synthetic. Native reach, the numeric helper body, complete COMPARE, readiness and credit remain unverified. The flat-RAM full load-word provider is not composed.
