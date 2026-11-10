# Synthetic COMPARE load-word classifier entry

The continuation starts at U25:0E0C, pushes BP, allocates six local bytes, rereads the address argument, and pushes the argument and CS. It checks the near call at 0E16 to U25:06FB, pushes return 0E19, and stops before the classifier. The SUB flags remain live. Malformed call checks retain prior effects; all stack accesses use the shared bus in source order.

The direct entry requires ordinary CS, DS and SS, SP in [000E, FFFA], 1 MiB RAM and attached EGA. The composition inherits the nested empty-session guards and frame, alias and immutable-code profile of [second-word entry](U25_SECOND_COMPARE_WORD_ENTRY_SHARED_BUS.md). Earlier stop and yield outcomes pass through. Synthetic tests cover the frame, flags, stack/data alias, malformed opcode and displacement, invalid entry and nested guards.

The sequence reuses [load-word evidence](U25_LOAD_WORD_0E0C.md) and the [shared-bus store classifier entry](U25_STORE_CLASSIFIER_ENTRY_SHARED_BUS.md). The complete load-word provider is a flat-RAM implementation; this continuation does not execute it. Prior SAVEs and operands are synthetic. Native reach, classifier return in this composition, full load-word return and complete COMPARE remain unverified.
