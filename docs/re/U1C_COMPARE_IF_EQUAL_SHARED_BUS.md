# Synthetic post-COMPARE IF-equal true path

The shared-bus continuation starts at U1C:0B52 with the return frame built by
[the IF entry](U1C_COMPARE_IF_ENTRY_SHARED_BUS.md). It pushes BP before
incrementing DS:49ED, reads the live opcode latch, and compares DS:6F78 for
opcode 16. A nonzero byte returns through the actual frame; a zero byte stops
at the skip call, and another latch stops at the next opcode comparison. These
stops preserve the effects already performed. Stack and data can alias.

The instruction ordering and predicate come from the existing recovered
[U1C IF-family contract](U1C_IF_0B52.md), also found through Dunking Sheep
memory context (PR 865). This bounded continuation does not execute the false
skip, other IF opcodes, or dispatcher instructions after the handler return.
The integration fixture and injected predicate are synthetic; no native reach
or full IF-family parity is claimed.
