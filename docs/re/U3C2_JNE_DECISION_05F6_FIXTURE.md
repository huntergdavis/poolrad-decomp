# U3C2 +05F6 supplied-FLAGS conditional gate

JNE reads only supplied ZF. With ZF clear it stops at +0600; with ZF set it
stops at +05F8. It executes no instruction at either destination and does not
infer that supplied FLAGS came from the preceding +05F3 comparison.

The 2-byte source extent is file [196936,196938), SHA256
`c8bc5eda4e19eb51796b9caed51ddd9921393a92a900634ead6e5c3e87fe732d`.
All supplied FLAGS, including AF, and all CPU words except IP remain unchanged;
the entire RAM image remains unchanged. RAM and the 14-word CPU object must be
distinct, with at least 1 MiB RAM, immutable code and no asynchronous traps.
Null pointers, short RAM and wrong IP reject atomically.

The synthetic C test covers all 65536 FLAGS words and compares all 14 CPU words,
with 32768 taken and 32768 fallthrough cases. The reference uses division/
modulo to extract ZF, independent of the implementation mask. Eight additional
full 1 MiB snapshots check supplied-state boundaries and argument rejection.
Sanitized runtime on this publication restack: 0.08 seconds.

## Source identity and prior work

Publication restack parent `76573656982561841104e74ef4b74b7c1eb9faeb`; PM i6964 authorized the scoped
restack and serial publication. The original local review head is
`3b3c1c25e6dd6defe9fe275edb91ee16aae9ec70`.

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
Source membership measured 255/255; metadata-only CMake checks measured
299 CTest registrations and 313 source paths, zero literal DEPENDS setters,
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
