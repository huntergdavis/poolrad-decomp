# U3C2 +0606: isolated supplied-state source fixture

JNE reads only supplied ZF: clear selects stop +0617; set selects stop +0608. Both destinations remain unexecuted. All supplied FLAGS, including AF, other CPU words and RAM retain their incoming values; only IP changes.

Raw RAM supplies at least 1048576 bytes; the distinct CPU object uses the shared fourteen uint16_t words. Code remains immutable, asynchronous/debug exceptions are excluded, and loaded CS/SS are supplied explicitly. Unit identity is not a loaded segment. Null pointers, short RAM, wrong entry IP reject before mutating valid supplied objects. Physical RAM aliases retain source-order behavior within this domain.

This fixture does not call a predecessor or successor. The existing +0603 fixture still stops at +0606. Neither this gate nor the other isolated gates extends the archival +057B raw prefix's first unsupported stop +0585. No completed predicate, return frame, RETF policy, natural-input provenance, joined function, native parity, strict credit or denominator change is established. Reserved +0906 and unrelated bodies remain outside scope.

## Pinned source and ownership

PM i7362 assigned this distinct source-only gate after PR641 merged. Independent branch parent 44dabf0b71acfe14bf54692b0c64b4606521f3ca is a fresh-main snapshot containing that merge. Before edits, the available ownership screen covered 29 clones, 710 worktree records and 6618 permitted U3C2 files plus named refs. Matches were predecessor/queued references and incidental media-size values; no distinct exact target implementation or named ref was found, with no read errors. Reserved paths were excluded before reads. This is an available-scope result, not a global ownership-absence claim.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction: file [196952,196954), 2 bytes, SHA256 bb52e9e6db182172c9062587fc4bdd0a673150f95815e0bcbae120469101acdd.
Original shape, zero calls/services and all successor instruction starts were rechecked statically before edits against the reviewed source map. Media, extracted instruction bytes and constructed loaded images remain ignored under resources/.

## Validation boundary

PM independently compiled and ran the exact focused synthetic C tests at reviewed heads a087845dba31d8a1cbc88e7327110c6615dc6ff2 (+0610) and 8e8887e2b2d01538f6cc26b2292468539a90e142 (+0606), serially at nice 19 with 20-second caps. PM review i7430 reports both exit 0, 1.7 seconds combined. PM also reports independent static original-source verification with SHA256 and objdump; this is code-identity evidence, not original/native execution. PM's Python lacked Capstone, so the worker's pinned static audit remains the Capstone decode gate.

Bounded focused synthetic C11/ASan/UBSan reruns, eleven Python image-metadata tests, pinned source hashes/static decode, constructed code-image match and changed-byte rejection, source membership, metadata-only CMake graph, notices and hygiene form the restack review gate. Exact-head measurements are recorded in the accompanying review packet. The heavy host-native hold remains: no emulator, original/native instruction execution, CMake configure, broad build or CTest run.

The synthetic C test contains all 65536 incoming FLAGS with independent arithmetic extraction of ZF, 32768 cases per outcome, complete 14-word CPU expectations and eight full 1 MiB RAM snapshots. PM executed these synthetic C cases at the exact reviewed implementation, as reported above. They do not measure original/native execution or a joined function.

The address-only static audit accepts --root, optional paired --loaded-image/--loaded-cs, and --output. A matching loaded instruction establishes only code identity; frame equality, producer consistency and execution are not inferred. Its synthetic metadata tests contain invented bytes only.

Bounded deja completed with no matches, searching zero indexed sessions; no indexed finding reused. Reused reviewed 1f9a4fc9336af2c7d411d3b9071d47c638b46a87 (+05F6 JNE raw guards, supplied FLAGS and independent reference tests), the shared raw CPU ABI, and entry4 map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Historical oracle/native findings were not rerun and supply no new original-execution evidence for this instruction.
