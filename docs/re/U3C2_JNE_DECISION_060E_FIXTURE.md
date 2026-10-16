# U3C2 +060E: isolated supplied-state JNE fixture

JNE reads supplied ZF: clear selects stop +0617; set selects stop +0610. Neither destination executes. Only IP changes; all supplied FLAGS, including AF, other CPU words and every RAM byte retain their incoming values.

The caller supplies at least 1048576 RAM bytes and a distinct CPU object with the shared fourteen uint16_t words. Code is immutable and asynchronous/debug exceptions are excluded. Null pointers, short RAM and wrong entry IP reject before mutation. The fixture neither reads a RAM operand nor constructs a caller frame.

## Source and ownership

PM i7878 assigned this distinct gate. Independent branch parent 0c3ddaf8c7f25cefbe8a10a5a81b9305f20b132c is the verified PR695 merge. Before edits, the available local screen covered 29 clones, 739 unique worktrees and 11652 permitted U3C2 file reads (450 unique contents). No dedicated target file or matching branch ref was found, with zero read errors. Twelve reference lines were preceding CMP fixture stop markers. Reserved paths were excluded before content reads. This describes available local evidence and does not establish global ownership absence.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction: file [196960,196962), 2 bytes, SHA256 22385441b9a81d8f8f35d17728263bf757d980dafe02b886cb4c0d4504501de5.
Original hashes, one-instruction JNE shape, zero calls/services and both successor instruction starts were checked statically against the reviewed entry4 map before edits. Original bytes and constructed RAM images remain ignored under resources/.

## Validation boundary

The focused synthetic C test checks all 65536 incoming FLAGS using arithmetic extraction of ZF, 32768 cases per outcome, complete fourteen-word CPU expectations and eight literal branch outcomes with full 1 MiB RAM snapshots. Nonperiodic RAM patterns expose unexpected writes. Invalid input cases check atomic rejection. These are C-only supplied-state tests.

Exact-head review records strict C11 warnings, bounded ASan/UBSan execution, eleven synthetic image-metadata tests, original-source static decode/hash checks, constructed loaded-code match and changed-byte rejection, hash-bound source membership, metadata-only CMake graph, notices and public-tree hygiene. The heavy host-native hold remains; no emulator, original/native execution, broad build, configure or CTest run is authorized by this fixture.

The static audit accepts --root, optional paired --loaded-image/--loaded-cs and --output. A matching loaded instruction establishes code identity only. It does not infer execution, upstream flag production, RAM/frame equality or predecessor consistency.

This isolated instruction does not join the preceding CMP fixture or execute any successor. The archival +057B prefix still stops at +0585. No completed predicate, return-frame policy, natural-input provenance, native parity, complete function, strict credit or denominator change follows from this gate. Reserved +0906 remains outside scope.

Bounded deja for +060E completed exit0 with no matches, searching zero indexed sessions; no indexed finding was reused. Reused reviewed +0606 JNE head 6b12790da9483af0c0744eb55488c04cb51ac958 for raw argument guards, supplied-ZF semantics and independent reference tests; the shared CPU ABI; and entry4 source map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Historical native findings were not rerun.
