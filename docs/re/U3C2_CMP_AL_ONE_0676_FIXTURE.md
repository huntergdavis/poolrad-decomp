# U3C2 +0676: isolated supplied-state source fixture

CMP AL,1 defines CF/PF/AF/ZF/SF/OF and stops at +0678 before JNE. FLAGS outside 08D5, AX, other CPU words and RAM are retained. It reads no RAM operand. Mandatory AF_CMP_DEFINED=1 selects the defined arithmetic flags.

The caller supplies at least 1048576 RAM bytes and a distinct shared fourteen-word CPU object. Code is immutable and asynchronous/debug exceptions are excluded. Null pointers, short RAM, wrong entry IP and unsupported policy reject before mutation. Loaded segments are supplied, not inferred from unit identity.

## Source and ownership

PM i7958 assigned two disjoint successor gates. This independent branch starts at fresh-main a540fab9f24874a7a335bc02e56a12c694df9e6e, containing verified PR707 merge f9e0c3bd2b941a9f1d5baa0e2994f9ebc2a408b3. Before edits, the available local screen covered 29 clones, 743 unique worktrees and 12348 permitted U3C2 reads (462 unique contents). No dedicated target filename, matching branch ref or read error was found. Eight references were preceding JNE stop markers. Reserved paths were excluded before content reads. This evidence does not establish global ownership absence.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction: file [197064,197066), 2 bytes, SHA256 2f46946da6a90ebf7ccc3efc5a332d1f50c87fa6619ba5db4e003faf7c9d0223.
Whole-media/entry4/target hashes, Capstone shape, zero calls/services and the following instruction start were checked against the reviewed map before edits. Original bytes and constructed RAM remain ignored under resources/.

## Validation boundary

Focused synthetic C tests cover 2048 AL/FLAGS-vector cases, all 65536 incoming FLAGS with coupled CPU values, and eight full 1 MiB nonperiodic RAM snapshots with literal arithmetic-flag pins. The independent reference uses arithmetic wrapping, unsigned/nibble borrow, signed-range overflow and bit-count parity. Full fourteen-word CPU expectations, RAM retention and atomic invalid-input rejection are checked.

Exact-head review records bounded strict C11/ASan/UBSan, eleven synthetic image-metadata tests, original/static identity, constructed code-image match/mismatch rejection, hash-bound source membership, metadata-only CMake graph, notices and hygiene. The heavy host-native hold remains: no original/native execution, emulator, broad build, CMake configure or CTest run.

The static audit accepts --root, optional paired --loaded-image/--loaded-cs and --output. Matching code establishes identity only, without execution, producer consistency or frame equality.

This isolated instruction neither joins its predecessor nor executes its successor. The archival +057B prefix still stops at +0585. No completed predicate, return-frame/RETF policy, natural-input provenance, native parity, complete function, strict credit or denominator change follows. Reserved +0906 remains outside scope.

Bounded deja completed exit0 with no matches, searching zero indexed sessions; no indexed finding reused. Reused d960fa494547ddb9c1fa22eaba6dd71ef87754c3 (+061A immediate/no-RAM guards and tests) and f21b1ed06b3664fb610470493a298b0e64227935 (+060B defined subtraction math), the shared CPU ABI, and source map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Historical native findings were not rerun.
