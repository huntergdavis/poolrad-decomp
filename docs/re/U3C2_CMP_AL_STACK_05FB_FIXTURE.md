# U3C2 +05FB supplied-state byte comparison

CMP AL,SS:[BP+08] computes the byte-subtraction flags and stops at +05FE,
before JE. Source file [196941,196944), SHA256
558f9ee8ee1f6a9e69bb28936ba299da128c363cc108b4d77bd9838af8e8ccfa.

The effective offset wraps to 16 bits before adding SS*16 and applying the
20-bit physical mask. Mandatory AF_CMP_DEFINED=1 computes CF/PF/AF/ZF/SF/OF;
other FLAGS, AX including AL, all other CPU words and RAM remain unchanged.
RAM and the 14-word CPU object must be distinct, with at least 1 MiB RAM,
immutable code and no asynchronous traps. Null/short RAM, wrong IP and
unsupported policies reject atomically.

The C test compares all 14 CPU words for all 65536 AL/memory pairs,
all 65536 incoming FLAGS words and all 65536 BP values coupled with SS/AX/FLAGS
permutations over nonperiodic RAM. Each sweep checks unchanged full RAM.
Eight full 1 MiB snapshots include literal offset/A20/code-address boundaries
with distinct DS/ES/SS. The independent reference uses signed-range overflow,
nibble borrow and bit-count parity. Policies 0/2/255/-1 and bad arguments
preserve state. Sanitized runtime on this publication restack: 0.28 seconds.

The API, arithmetic reference approach and metadata guards reuse the reviewed
[+05F3 comparison](U3C2_CMP_AL_STACK_05F3_FIXTURE.md), merged exact head
4a86cc16ea31ff3e23b5f69a9a8e1ace0f7d5eda. This slice has its own original
start, displacement, source hash and stop.

## Source and ownership gates

Publication restack parent 2747fa05a949057b813325a201de2f22faced445; PM i7077
authorized the scoped serial restack, exact pre-SSH handoff and publication.
The reviewed original head is 34bbfe5dd2cf0cd7bc2c9793be0495d449b67509.

Original review parent 7fd63488ebd2672e2f51d4b970a4b1b47710ed6c, after verified PR603 merge. The #424 assignment
authorizes two independent local heads and requires exact PM review before SSH.
Source map reused from reviewed entry4 head
4313dcbbc6e981477f252e1f7ddacab2c4e9931c.

GAME.OVR: 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 [057B,0909), file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Original instruction width, shape and hash were checked before edits.

Bounded recall used timeout 12s with query
"U3C2 05FB CMP 0600 loaded byte BP minus 2 source-only ownership".
Exit 124 is an incomplete indexed search, not a no-match finding.
The available ownership screen covered 2938 permitted U3C2 files from the
24-clone discovery plus fresh census worktrees and local/origin-tracking refs.
Its 72 text matches reduced to 23 unique lines, including one incidental
numeric match; the address references were predecessor stops and queued
proposals. No distinct implementation or exact named ref was found; zero
errors. Reserved paths were excluded before content reads. This is not
global proof of absent ownership.

## Verification and limits

Each branch passed 11 synthetic loaded-image metadata tests, media/body/source
hashes, exact instruction decode, a constructed 1 MiB code-image match and
rejection after changing one instruction byte. Instruction identity establishes
neither image provenance nor actual caller/frame equality.

Focused C passed C11 with -Wall -Wextra -Wpedantic -Werror and ASan/UBSan,
serially under nice -n 19 / timeout 20s. Hash-bound recovered source membership was
261/261 per independent branch. Metadata-only CMake checks measured
306 CTest registrations and 324 source paths, 0 literal DEPENDS setters,
and an acyclic graph. Parent CMake restoration, notices, script syntax-only
checks and diff hygiene passed.

Original bytes, constructed images, binaries and logs stay ignored in resources.
No emulator, Unicorn, original instruction execution, native oracle, configure
or broad build ran. The heavy host-native hold remains. These raw supplied-state
fixtures establish no predecessor consistency, inter-fixture join, branch
successor execution, return, game identity or complete-function translation.
Function credit/accounting and denominator records are unchanged.

Queued later leads only: +05FE JE stopping +0610 or +0600, and +0603 CMP
stopping +0606. Their own ownership/source/PM gates precede implementation.
Exact committed local heads must receive PM review before any SSH publication.
