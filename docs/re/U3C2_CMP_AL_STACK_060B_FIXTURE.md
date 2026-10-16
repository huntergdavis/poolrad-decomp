# U3C2 +060B: isolated supplied-state comparison

CMP AL,SS:[BP+08] computes defined byte-subtraction CF/PF/AF/ZF/SF/OF and stops before +060E JNE. The SS effective offset wraps to 16 bits before segment-base addition and A20 masking. Mandatory AF_CMP_DEFINED=1 selects the source-defined arithmetic flags. FLAGS outside 08D5, AX and every other CPU word and RAM byte are retained. Both following branches and their destinations remain unexecuted.

Raw RAM provides at least 1048576 bytes and is distinct from the fourteen-word CPU. Enter only at +060B with explicit loaded segments, immutable original code and no asynchronous/debug traps. Null pointers, short RAM, wrong IP or unsupported policy reject before mutation. Supplied state does not establish its caller frame or predecessor consistency. The earlier +0608/+0617 loads remain separate; no load-to-compare join, predicate result, RETF policy, natural provenance, native/full-function or strict credit is established. The archival +057B prefix still stops unsupported at +0585.

## Source and ownership

PM i7712 assigned two distinct fresh-main local gates after PR675 merged; exact PM review remains required before SSH. Parent 69b5c79e0aaa6e443b9e361904461fd280b0e55f contains that merge. Before edits, the available screen covered 31 clones, 729 worktree records and 9336 permitted U3C2 files plus named refs. Matches describe prior load stops; no dedicated target implementation, named ref, candidate filename or read error was found. Reserved paths were excluded before reads. This is not a global ownership-absence certificate.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction: file [196957,196960), 3 bytes, SHA256 558f9ee8ee1f6a9e69bb28936ba299da128c363cc108b4d77bd9838af8e8ccfa.
Exact shape, zero calls/services and the stop instruction start were statically verified before source edits. Original media, extracted bytes and constructed code images remain ignored under resources/.

## Focused validation

Synthetic C tests cover: All 65536 AL/memory-byte pairs, all 65536 incoming FLAGS, 65536 coupled BP/SS/AX/FLAGS states and eight literal full 1 MiB RAM snapshots. Independent reference uses signed-range overflow, nibble borrow and bit-count parity. The final literal source-read address is SS=1404/BP=0603 -> physical 83531. Unsupported policies and null/short/wrong-IP inputs reject atomically.

Review gates use bounded strict C11 warnings with ASan/UBSan, eleven synthetic Python image-metadata tests, pinned original hashes/static decode, constructed code-image match/changed-byte rejection, recovered source membership, metadata-only CMake graph, notices and hygiene. Exact measured results belong to the head-specific review packet. No original/native instruction, emulator, broad build/configure or CTest runs; heavy host-native hold remains.

The static audit accepts --root, optional paired --loaded-image/--loaded-cs and --output. Code identity establishes no producer/frame equality, caller reach or branch execution. Synthetic metadata tests contain invented bytes only.

Both bounded deja queries completed with no matches, searching zero indexed sessions; no recalled finding reused. Reused reviewed +05FB comparison 53c47ca9820562486e88b092aa1388c5e93cb85a for raw guards, flag conventions and independent subtraction tests; shared CPU ABI; and entry4 map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Immediate-zero expectations are independently specialized to fixed-zero CF/AF/OF. No historical oracle/native finding is rerun or promoted, and separate U6F static coordination supplies no evidence for this gate.
