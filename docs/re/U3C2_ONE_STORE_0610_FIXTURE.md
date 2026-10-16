# U3C2 +0610: isolated supplied-state source fixture

MOV byte SS:[BP-1],1 writes exactly one supplied-RAM byte, then stops before +0614. The effective offset wraps to 16 bits before SS-base addition and A20 masking. Mandatory AF_PRESERVE_SUPPLIED=1 preserves every supplied FLAGS bit; the upstream native AF value remains unknown. All CPU words except IP retain their supplied values.

Raw RAM supplies at least 1048576 bytes; the distinct CPU object uses the shared fourteen uint16_t words. Code remains immutable, asynchronous/debug exceptions are excluded, and loaded CS/SS are supplied explicitly. Unit identity is not a loaded segment. Null pointers, short RAM, wrong entry IP or unsupported AF policy reject before mutating valid supplied objects. Physical RAM aliases retain source-order behavior within this domain.

This fixture does not call a predecessor or successor. The existing +0603 fixture still stops at +0606. Neither this gate nor the other isolated gates extends the archival +057B raw prefix's first unsupported stop +0585. No completed predicate, return frame, RETF policy, natural-input provenance, joined function, native parity, strict credit or denominator change is established. Reserved +0906 and unrelated bodies remain outside scope.

## Pinned source and ownership

PM i7362 assigned this distinct source-only gate after PR641 merged. Independent branch parent 44dabf0b71acfe14bf54692b0c64b4606521f3ca is a fresh-main snapshot containing that merge. Before edits, the available ownership screen covered 29 clones, 710 worktree records and 6618 permitted U3C2 files plus named refs. Matches were predecessor/queued references and incidental media-size values; no distinct exact target implementation or named ref was found, with no read errors. Reserved paths were excluded before reads. This is an available-scope result, not a global ownership-absence claim.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction: file [196962,196966), 4 bytes, SHA256 14470acec7c07571af0efb073e074fab9fb7da33c4f9f14d7fdf22181ddc8ee2.
Original shape, zero calls/services and all successor instruction starts were rechecked statically before edits against the reviewed source map. Media, extracted instruction bytes and constructed loaded images remain ignored under resources/.

## Validation boundary

PM independently compiled and ran the exact focused synthetic C tests at reviewed heads a087845dba31d8a1cbc88e7327110c6615dc6ff2 (+0610) and 8e8887e2b2d01538f6cc26b2292468539a90e142 (+0606), serially at nice 19 with 20-second caps. PM review i7430 reports both exit 0, 1.7 seconds combined. PM also reports independent static original-source verification with SHA256 and objdump; this is code-identity evidence, not original/native execution. PM's Python lacked Capstone, so the worker's pinned static audit remains the Capstone decode gate.

Bounded focused synthetic C11/ASan/UBSan reruns, eleven Python image-metadata tests, pinned source hashes/static decode, constructed code-image match and changed-byte rejection, source membership, metadata-only CMake graph, notices and hygiene form the restack review gate. Exact-head measurements are recorded in the accompanying review packet. The heavy host-native hold remains: no emulator, original/native instruction execution, CMake configure, broad build or CTest run.

The synthetic C test contains 558 full 1 MiB RAM/14-word CPU snapshots, including eight independently pinned offset-underflow/A20 addresses, all 65536 incoming FLAGS and 65536 coupled BP/SS/AX/FLAGS cases. It checks the constant-one write, independent address arithmetic and atomic rejection. PM executed these synthetic C cases at the exact reviewed implementation, as reported above. They do not measure original/native execution or a joined function.

The address-only static audit accepts --root, optional paired --loaded-image/--loaded-cs, and --output. A matching loaded instruction establishes only code identity; frame equality, producer consistency and execution are not inferred. Its synthetic metadata tests contain invented bytes only.

Bounded deja completed with no matches, searching zero indexed sessions; no indexed finding reused. Reused reviewed 75dd378198d0d2039eabd12d74d63f8f710daf62 (+05B1 zero-store raw guards, addressing and reference tests), the shared raw CPU ABI, and entry4 map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Historical oracle/native findings were not rerun and supply no new original-execution evidence for this instruction.
