# Synthetic COMPARE second-slot word entry

The bounded word path begins at U25:017A. It reads each live stack local in source order, zero-extends the bytes, assembles the address, sets the final ADD flags, pushes the address and CS, and checks the near call at 018E to 0E0C. It pushes return 0191 and stops at the load-word entry without executing it. A malformed call retains the preceding stack and register effects. The caller owns immutable code outside the writable stack footprint.

The direct entry requires ordinary CS, DS and SS, BP at least 16, SP=BP-10, 1 MiB RAM, and attached EGA. The composition inherits the frame, alias and nonempty-session bounds of [second-slot dispatch](U25_SECOND_COMPARE_DISPATCH_SHARED_BUS.md); other dispatch outcomes and earlier stops pass through. Standalone tests cover the frame, flags, stack/data alias, malformed opcode and displacement, and invalid-frame atomicity. The synthetic integration covers the second slot word path.

The ordered body and call target reuse [U25 slot-value evidence](U25_SLOT_VALUE_0136.md) and [pair-call boundary evidence](U25_SLOT_PAIR_ENTRY_SHARED_BUS.md). The preceding SAVEs and operands are synthetic; native reach, the load-word body, complete COMPARE, readiness and credit remain unverified.
