# U3C2 +0621: isolated supplied-state CMP fixture

CMP AL,SS:[BP-02] computes defined byte-subtraction CF/PF/AF/ZF/SF/OF and stops at +0624 before JL. Its effective SS offset wraps to 16 bits before segment-base addition and A20 masking. Mandatory AF_CMP_DEFINED=1 selects the defined arithmetic flags; FLAGS outside 08D5, AX, other CPU words and all RAM are retained.

The caller supplies at least 1048576 RAM bytes and a distinct shared fourteen-word CPU object. Code is immutable and asynchronous/debug exceptions are excluded. Null pointers, short RAM, wrong entry IP and unsupported policy reject before mutation. Loaded segment values are supplied explicitly, not inferred from unit identity.

## Source and ownership

PM i8076 assigned this separate source gate. Independent parent de68e63bc939e6d9ee791894f05ad2be939b541d is the verified PR724 merge. Before edits, the available screen covered 32 clones, 758 unique worktrees and 14997 permitted U3C2 reads (474 unique contents). No dedicated target filename, matching branch ref or read error was found. Fourteen references were preceding load/CMP stop markers. Reserved paths were excluded before reads. This is local evidence, not global ownership absence.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction: file [196979,196982), three bytes, SHA256 f2eb956722e6012b0ef260dcd57c4228296c0acbfc6eb29fc0ad8f3b4b98d2ce.
Whole-media/entry4/target hashes, Capstone shape, zero calls/services and the following instruction start were pinned against the reviewed map before edits. Original bytes and constructed RAM remain ignored under resources/.

## Validation boundary

Synthetic C tests cover all 65536 AL/memory-byte pairs, all 65536 incoming FLAGS, all 65536 coupled BP/SS/AX/FLAGS values and eight literal full 1 MiB RAM snapshots. Independent negative-offset/A20/alias pins include SS0000/BP0001 -> 65535, SSFFFF/BP0000 -> 65518 and SS1404/BP0623 -> 83553. The arithmetic reference uses signed-range overflow, nibble borrow and bit-count parity. Complete fourteen-word CPU expectations, RAM retention and atomic rejection are checked.

Exact-head review records bounded strict C11/ASan/UBSan, eleven synthetic image-metadata tests, original/static identity, constructed code-image match/mismatch rejection, hash-bound source membership, metadata-only CMake graph, notices and hygiene. No original/native execution, emulator, broad build, CMake configure or CTest run. Heavy host-native hold retained.

The static audit accepts --root, paired --loaded-image/--loaded-cs and --output. Matching code establishes identity only, without execution, producer consistency or frame equality. This instruction neither joins its predecessor nor executes its successor; the archival +057B prefix still stops at +0585. No completed predicate, RETF policy, natural-input provenance, native parity, complete function, strict credit or denominator change follows. Reserved +0906 remains outside scope.

Both queued and refreshed bounded deja queries timed out after 12 seconds (exit124). Recall is incomplete, not no-match evidence; no indexed finding was reused. Reused reviewed +060B f21b1ed06b3664fb610470493a298b0e64227935 subtraction math, raw guards and independent reference tests; shared CPU ABI; and source map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Historical native findings were not rerun.
