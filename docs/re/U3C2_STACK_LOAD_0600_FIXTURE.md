# U3C2 +0600 supplied-state stack-byte load

MOV AL,SS:[BP-02] loads one byte and stops at +0603 before CMP.
Source file [196946,196949), SHA256
a757cd3fcc025353ab4a54f131bfab01f6cd2f78e1e3923509b8cad45ed3f885.

The BP-2 effective offset wraps to 16 bits before adding SS*16 and applying
the 20-bit physical mask. AL changes; AH, every other CPU word, all FLAGS and
the full RAM image remain unchanged. Mandatory AF_PRESERVE_SUPPLIED=1 retains
supplied synthetic AF without establishing upstream native AF. RAM and the
14-word CPU object must be distinct, with at least 1 MiB RAM, immutable code
and no asynchronous traps. Null/short RAM, wrong IP and unsupported policies
reject atomically.

The C suite checks 558 full 1 MiB snapshots: 512 byte/FLAGS cases, 30 SS/BP
boundaries, 8 code-address aliases and 8 literal underflow/A20 cases. For example,
SS=FFFF/BP=0000 reads physical 0FFEE after offset underflow; SS=FFFF/BP=0012
reads physical 00000 after A20 wrap. The reference uses (BP+65534) modulo 65536,
independent of the implementation's truncated subtraction.
A further 65536 coupled cases cover every BP/SS/AX/FLAGS value over nonperiodic
RAM, with full CPU comparison per case and full unchanged-RAM comparison after
the sweep. These are coupled permutations, not a Cartesian cross-product.
Policies 0/2/255/-1 and bad arguments preserve state.
Sanitized runtime on this publication restack: 0.45 seconds.

The API, whole-state test approach and metadata guards reuse the reviewed
[+05F8 load](U3C2_STACK_LOAD_05F8_FIXTURE.md), merged exact head
ee4c3b77a90c24b22a35d6492fb5024b1d93fb16. The negative displacement and its
literal wrap cases are independently checked for this original start.

## Source and ownership gates

Publication restack parent baa0b13a56f2fc83c1420f5c6e4743b4a9749e18; PM i7077
authorized the scoped serial restack, exact pre-SSH handoff and publication.
The reviewed original head is 22ad46eeb1228170be5da0fe87431994bb84f1b3.

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
263/263 per independent branch. Metadata-only CMake checks measured
308 CTest registrations and 326 source paths, 0 literal DEPENDS setters,
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
