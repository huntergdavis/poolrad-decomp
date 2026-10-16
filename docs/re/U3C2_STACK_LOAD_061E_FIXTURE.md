# U3C2 +061E: isolated supplied-state source fixture

MOV AL,SS:[BP+0A] reads one byte and stops at +0621 before CMP. AH, all supplied FLAGS and every other CPU word and RAM byte are retained. Its effective offset wraps to 16 bits before SS-base addition and A20 masking. Mandatory AF_PRESERVE_SUPPLIED=1 retains supplied AF.

The caller supplies at least 1048576 RAM bytes and a distinct shared fourteen-word CPU object. Code is immutable and asynchronous/debug exceptions are excluded. Null pointers, short RAM, wrong entry IP and unsupported policy reject before mutation. Loaded segments are supplied, not inferred from unit identity.

## Source and ownership

PM i7958 assigned two disjoint successor gates. This independent branch starts at fresh-main a540fab9f24874a7a335bc02e56a12c694df9e6e, containing verified PR707 merge f9e0c3bd2b941a9f1d5baa0e2994f9ebc2a408b3. Before edits, the available local screen covered 29 clones, 743 unique worktrees and 12348 permitted U3C2 reads (462 unique contents). No dedicated target filename, matching branch ref or read error was found. Eight references were preceding JNE stop markers. Reserved paths were excluded before content reads. This evidence does not establish global ownership absence.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction: file [196976,196979), 3 bytes, SHA256 8893437474228ab2282eb2c92ba1b0fec02b0c76518f0739ed364b093616fff6.
Whole-media/entry4/target hashes, Capstone shape, zero calls/services and the following instruction start were checked against the reviewed map before edits. Original bytes and constructed RAM remain ignored under resources/.

## Validation boundary

Focused synthetic C tests cover 558 full 1 MiB RAM comparisons, including all byte values, segment/offset edges, eight instruction-address aliases and eight literal wrap/A20/alias pins; all 65536 coupled BP/SS/AX/FLAGS values on a nonperiodic RAM background. Full fourteen-word CPU expectations, RAM retention and atomic invalid-input rejection are checked.

Exact-head review records bounded strict C11/ASan/UBSan, eleven synthetic image-metadata tests, original/static identity, constructed code-image match/mismatch rejection, hash-bound source membership, metadata-only CMake graph, notices and hygiene. The heavy host-native hold remains: no original/native execution, emulator, broad build, CMake configure or CTest run.

The static audit accepts --root, optional paired --loaded-image/--loaded-cs and --output. Matching code establishes identity only, without execution, producer consistency or frame equality.

This isolated instruction neither joins its predecessor nor executes its successor. The archival +057B prefix still stops at +0585. No completed predicate, return-frame/RETF policy, natural-input provenance, native parity, complete function, strict credit or denominator change follows. Reserved +0906 remains outside scope.

Bounded deja completed exit0 with no matches, searching zero indexed sessions; no indexed finding reused. Reused 57f2003a89e51ac2014d127e5907f4d2d8952c89 (+0617 load guards, address wrapping and independent literal/reference tests), the shared CPU ABI, and source map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Historical native findings were not rerun.
