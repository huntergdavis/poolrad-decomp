# Synthetic COMPARE second slot-value return

The bounded U25:0191 continuation stores AX at SS:BP-2, reloads that live word, restores SP and BP, and performs RETF 2 on the shared bus. Flags remain unchanged. The direct provider checks returned CS, IP, SP, BP and SS against an explicit target; a mismatch reports the transfer after executed effects. It requires ordinary CS, DS and SS, BP in [000A, FFF8], SP=BP-10, 1 MiB RAM and attached EGA.

The composed wrapper extends the [low load-word return](U25_COMPARE_LOAD_LOW_RETURN_SHARED_BUS.md) to the checked U1C:0191 caller return using a copied target and the established COMPARE frame geometry. Other stops and yields pass through; inherited alias, immutable-code and nested-session constraints remain in force. Synthetic integration and standalone tests cover the result, flags, alias, transfer and invalid frame. The sequence was decoded read-only from ignored owner media and agrees with [slot-value evidence](U25_SLOT_VALUE_0136.md) and the existing [pair-slot return](U25_SLOT_VALUE_RETURN_SHARED_BUS.md).

Prior SAVEs and operands are synthetic. Native reach, the high class-four path, subsequent COMPARE operations, complete COMPARE, readiness and credit remain unverified. The flat-RAM full load-word provider is not composed.
