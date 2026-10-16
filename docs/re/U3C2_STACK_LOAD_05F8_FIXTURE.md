# U3C2 +05F8 supplied-state stack-byte load

MOV AL,SS:[BP+0C] loads one byte, then stops at +05FB before CMP.
The 3-byte source extent is file [196938,196941), SHA256
`74d3cf8eaa53746dbad6b7bd6f2fb6131a6354699c407927f5a84c56f7d061b3`.
An equal instruction hash elsewhere does not merge different original starts.

The effective offset wraps to 16 bits before adding SS*16 and masking to 20
physical address bits. AL changes; AH, every other CPU word, all FLAGS and
the full RAM image remain unchanged. Mandatory AF_PRESERVE_SUPPLIED=1 retains
synthetic supplied AF without establishing upstream native AF. RAM and CPU
must be distinct, with at least 1 MiB RAM, immutable code and no asynchronous
traps. Null pointers, short RAM, wrong IP and unsupported policies reject
atomically.

The synthetic C test checks 556 full 1 MiB snapshots: 512 byte/FLAGS cases,
30 segment/offset boundaries, eight code-address aliases and six independently
calculated literal offset/A20 boundaries. A further 65536 cases cover every
BP/SS/AX/FLAGS value through coupled permutations over nonperiodic RAM, with
full CPU comparison per case and full unchanged-RAM comparison after the sweep.
These coupled cases are not a Cartesian cross-product. Invalid policies
0/2/255/-1 and argument errors preserve state. Sanitized runtime on this publication restack: 0.60 seconds.

## Source identity and prior work

Publication restack parent `3d20bf58d8ee00342afcaa8f9ff8692283c25312`; PM i6964 authorized the scoped
restack and serial publication. The original local review head is
`455b6fad875d8e885377f38b3335822724244658`.

Original review parent `0719e7350e16e1243f87b7939cc25a9f2155c402`; PM i6916 assigned these local
supplied-state scopes after PR595 merged. The reviewed entry4 static map is
`4313dcbbc6e981477f252e1f7ddacab2c4e9931c`.
GAME.OVR is 206105 bytes, SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Entry4 [057B,0909), file [196813,197723), SHA256
`59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae`.

Bounded recall `timeout 12s deja "U3C2 05F6 JNE 05F8 stack load supplied-state ownership"`
ended 124: the indexed search is incomplete. Reused findings are cited here.
The conditional API and whole-CPU test approach reuse merged +05AF at
`6fdc1f388bbe266aad6968ff0752046b88305e2a`; the stack-load API, source/image guards and synthetic metadata
tests reuse merged +05E7 at `fa5a156a0706421ec64a186ba7844ef2e34fb8c2`.

The available-scope screen examined 2645 permitted U3C2 files from the cached
24-clone discovery plus fresh census worktrees. Its 22 references reduced to
10 unique basename/text pairs: predecessor stops and queued proposals, with
no distinct implementation found and zero read errors. Reserved paths were
excluded before reads. This is not a global ownership-absence claim.

## Measured checks and limits

Each branch passed 11 synthetic metadata tests, media/body/instruction hashes,
a single-instruction static decode, a constructed 1 MiB code-image match and
changed-instruction-byte rejection. Code identity does not establish RAM frame
equality or input provenance. Original bytes, constructed RAM, binaries and
logs remain ignored under resources.

Each focused C suite passed C11
`-Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined`,
serially under `nice -n 19 timeout 20s`.
Source membership measured 257/257; metadata-only CMake checks measured
301 CTest registrations and 315 source paths, zero literal DEPENDS setters,
and an acyclic graph. Notice hashes, oracle-script syntax-only checks, CMake
preservation and diff hygiene passed.

No original instructions, native oracle, emulator, Unicorn, configure or broad
build ran. No predecessor consistency, inter-fixture join, caller-frame return
or whole-function translation is claimed. Strict accounting is unchanged at
the reviewed floor of 82, and the whole-game denominator stays unknown.
The reserved +0906 RETF and rejected +0907 immediate data remain untouched.

Queued later leads: +05FB CMP AL,SS:[BP+08] stopping +05FE, and +0600
MOV AL,SS:[BP-02] stopping +0603. Each needs its own ownership/source/PM gates;
neither is reserved by this document. Exact local heads go to PM before SSH.
