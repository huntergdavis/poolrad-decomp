# Synthetic COMPARE load-word class-four low path

The bounded continuation reads the live address at U25:0EA7 and compares it with C04B. High addresses stop at 0EEE before their body. Low addresses perform the original ordered comparisons and mapped reads for 00B1, 00FB, 00FC and 033D, then stop at 0F4A before the load-word epilogue. Unmapped low addresses, including 035F and the synthetic 2208, leave the existing SS:BP-2 result word untouched. The final comparison or XOR flags remain live; reads and writes use the shared bus.

The direct entry requires ordinary CS, DS and SS, BP at least 6, SP=BP-6, 1 MiB RAM and attached EGA. The wrapper inherits the [load dispatch](U25_COMPARE_LOAD_DISPATCH_SHARED_BUS.md) frame, alias, immutable-code and nested-session profiles, passing other stops through. Standalone cases cover mapped addresses, unmapped values, the C04B boundary, stack/data alias and invalid-frame atomicity. Synthetic integration reaches the unmapped low stop. Instruction boundaries were decoded from ignored owner media; [class-four evidence](ECL_LOAD_WORD_CLASS4.md) describes the mapped and untouched-local behavior. No game bytes are included.

The preceding SAVEs and operands are synthetic. Native reach, the high body, epilogue, full shared-bus load-word return, completed COMPARE, readiness and credit remain unverified. The flat-RAM load-word provider is not composed.
