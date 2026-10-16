# U3C2 +0617: isolated supplied-state stack-byte load

MOV AL,SS:[BP+06] loads one byte and stops before +061A CMP. AH, all supplied FLAGS and every other CPU word and RAM byte are retained. The effective SS offset wraps to 16 bits before segment-base addition and A20-off physical masking. Mandatory AF_PRESERVE_SUPPLIED=1 retains supplied AF; no upstream native AF value is inferred.

Raw RAM provides at least 1048576 bytes, distinct from the shared fourteen-word CPU object. Enter only at +0617 with explicit loaded segments and immutable original code; asynchronous/debug traps are outside scope. Null pointers, short RAM, wrong entry IP or unsupported policy reject atomically. No predecessor, successor or frame composition executes. The existing +0606 gate still stops at +0608/+0617. This isolated load does not extend the archival +057B prefix's first unsupported stop +0585, complete a predicate, select a RETF policy, establish natural provenance or earn native/full-function/strict credit.

## Source and ownership

PM i7595 authorizes two separate local load gates after read-only scope review i7585; exact PM review remains required before SSH. Independent branch parent 6e9ae8ba5b703deed6a3e2c22862f21cd1611f21 is a fresh-main snapshot containing PR658's merge. Before source edits, the screen covered 29 available clones, 718 worktree records and 7809 permitted U3C2 files plus named refs. The 82 references/11 unique lines describe predecessor stops and source ranges; no distinct target implementation, exact named ref, candidate filename or read error was found. Reserved paths were excluded before content reads. This does not certify global ownership absence.

GAME.OVR: 206105 bytes, SHA256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4: file [196813,197723), SHA256 59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
This instruction: file [196969,196972), 3 bytes, SHA256 bfc817306da8d0f9e493d69d813124a3160e557a45e26df9156e1f3672415e37.
Pinned source hash/shape, zero calls/services and the stop instruction start were independently verified before edits. Original media, extracted code and constructed loaded images stay ignored under resources/.

## Focused validation

The synthetic C test checks 558 full 1 MiB RAM/14-word CPU snapshots: every loaded byte under both all-clear/all-set FLAGS, 30 segment/offset boundaries, eight read-address aliases and eight independently pinned literal wrap/A20 addresses. It also checks 65536 coupled BP/SS/AX/FLAGS states against independent modulo address arithmetic and high-byte preservation, plus atomic invalid-input/policy rejection. Expected literal addresses are constants, not computed by the fixture. A nonperiodic RAM background detects missing 64 KiB offset wrap.

Review gates are bounded strict C11 warnings with ASan/UBSan, eleven synthetic Python image-metadata tests, pinned media/body/instruction hash and static decode, constructed code-image match/changed-byte rejection, source membership, metadata-only CMake graph, notices and hygiene. Exact measured results are in the head-specific review packet. No original/native instruction, emulator, broad build/configure or CTest runs; the heavy host-native hold remains.

The static audit accepts --root, optional paired --loaded-image/--loaded-cs, and --output. Code identity does not imply producer/frame equality, successor execution or a reached game route. Synthetic metadata tests contain invented bytes only.

Bounded deja for both successor addresses completed with no matches, searching zero indexed sessions. No recalled finding was reused. Reused reviewed +0600 load 3d82b775cade999c7763c6e8a75730cc098245c1 for guards, CPU/RAM expectations and independent underflow tests; +05F8 load ee4c3b77a90c24b22a35d6492fb5024b1d93fb16 for positive-offset boundary conventions; shared raw CPU ABI; and entry4 map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c. No historical native/oracle finding was rerun or promoted. Separate U6F ownership work supplies no evidence for this fixture.
