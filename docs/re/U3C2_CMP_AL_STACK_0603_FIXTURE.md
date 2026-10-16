# U3C2 +0603 supplied-state byte comparison

CMP AL,SS:[BP+0A] computes byte-subtraction flags and stops at +0606 before JNE.
Source file [196949,196952), SHA256
2ad72c7b85ea62f631525de59646f9e9a5d2e70192384c99207d2d0eb9c8a68c.
Equal instruction bytes at another PC do not merge original starts.

The effective offset wraps to 16 bits before adding SS*16 and masking to 20 bits.
Mandatory AF_CMP_DEFINED=1 computes CF/PF/AF/ZF/SF/OF; other FLAGS, AX including
AL, all other CPU words and RAM remain unchanged. RAM and the 14-word CPU object
must be distinct, with at least 1 MiB RAM, immutable code and no asynchronous
traps. Null/short RAM, wrong IP and unsupported policies reject atomically.

The synthetic C suite checks all 65536 AL/memory pairs, all 65536 incoming FLAGS
and all 65536 BP values coupled with SS/AX/FLAGS permutations over nonperiodic
RAM. Every case compares the full CPU; every sweep checks unchanged full RAM.
Eight full 1 MiB snapshots include literal offset/A20/code-address boundaries
and distinct DS/ES/SS. The independent reference uses signed-range overflow,
nibble borrow and bit-count parity. Unsupported policies 0/2/255/-1 and bad
arguments preserve state. Sanitized runtime: 0.21 seconds.

The API, arithmetic reference and metadata guards reuse reviewed
[+05F3 CMP](U3C2_CMP_AL_STACK_05F3_FIXTURE.md), exact merged head
4a86cc16ea31ff3e23b5f69a9a8e1ace0f7d5eda. The original start, stop and code-address
literal are independently pinned for +0603.

## Original source, ownership and dependencies

Parent 7167b0e1b241eba90850bd9a7561d1bed8f1d368; exact PR623 merge verified as ancestor.
PM i7166 assigns independent local source-only branches and exact local review.
Original GAME.OVR is 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 [057B,0909), file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Original instruction hash, width, shape and successor boundaries were checked
before edits using the entry4 map reviewed at
4313dcbbc6e981477f252e1f7ddacab2c4e9931c. Each assigned slice has zero calls,
interrupts or external services. Its successor instruction starts are verified;
their bodies do not execute in the fixture.

Bounded recall used timeout 12s with query
"U3C2 05FE JE 0603 CMP bounded supplied state ownership"; exit 124 means the
indexed search is incomplete, not a no-match finding. Reused sources are cited.
The available screen covered 3271 permitted U3C2 files from cached 24-clone
discovery plus fresh census worktrees and local/origin-tracking refs. Its 70 text
matches reduced to 24 unique lines: predecessor stops, queued proposals and
incidental PR603 references. No distinct implementation or exact named ref was
found; zero errors. Reserved paths were excluded before content reads.
This is an available-scope ownership check, not global absence proof.

## Verification and limits

Each branch passed 11 synthetic loaded-image metadata tests, exact original
media/body/instruction hash+decode, a constructed 1 MiB code-image match and
changed-instruction-byte rejection. Instruction identity proves neither image
origin nor actual original caller/frame equality.

Focused C passed C11 -Wall -Wextra -Wpedantic -Werror with ASan/UBSan,
serially under nice -n 19 / timeout 20s. Hash-bound recovered source membership
was 266/266 per independent branch. Metadata-only CMake checks measured 311 CTest
registrations and 329 source paths, zero literal DEPENDS setters, and an acyclic
graph. Parent CMake restoration, notice hashes, syntax-only and diff checks
passed.

Original/derived bytes, images, binaries and logs remain ignored in resources.
No original instruction execution, emulator, Unicorn, native oracle, configure
or broad build ran; heavy host-native hold remains. No predecessor/producer
join, caller-frame return, new original callable identity, whole-function or
strict credit is claimed. Accounting and denominator records stay untouched.

Queued later leads only: +0610 MOV byte SS:[BP-1],1 stopping +0614, and
+0606 JNE stopping +0617 or +0608. Each requires its own ownership/source/PM gate.
These leads are not reserved. Exact local heads go to PM before SSH publication.
