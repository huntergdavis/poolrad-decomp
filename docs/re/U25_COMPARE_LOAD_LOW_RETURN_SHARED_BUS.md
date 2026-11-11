# Synthetic COMPARE load-word low return

The bounded epilogue at U25:0F4A reads the live SS:BP-2 result word into AX, restores SP and BP, then performs RETF 2 on the shared bus. Flags remain unchanged. The direct provider checks returned CS, IP, SP, BP and SS against the caller's target and reports a transfer after the actual effects if the frame differs. It requires ordinary CS, DS and SS, BP in [0006, FFF8], SP=BP-6, 1 MiB RAM and attached EGA.

The composed wrapper extends the [class-four low path](U25_COMPARE_LOAD_CLASS4_LOW_SHARED_BUS.md) only to the checked U25:0191 return. High addresses and prior outcomes pass through. The expected frame follows the bounded caller's actual stack geometry; inherited alias, immutable-code and nested-session constraints remain in force. Standalone tests cover the live result, flags, alias, transfer and invalid-frame atomicity; synthetic integration checks the returned live local. The instruction sequence was decoded read-only from ignored owner media and agrees with [load-word evidence](U25_LOAD_WORD_0E0C.md).

Prior SAVEs and operands are synthetic. Native reach, the high class-four path, subsequent slot-value and COMPARE operations, complete COMPARE, readiness and credit remain unverified. The flat-RAM full load-word provider is not composed.
