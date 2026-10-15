# Saved slot-A post-Up offscreen first 43-row gate

**Classification:** bounded C page-state model of the reached 43-row
`0C88:2632` mode-one path. It has direct native parity for the entire
offscreen object, dirty/extent metadata, and loop counter. It does not model
CPU registers or runtime stack residue, claim a complete static frame, or
establish a first writer across every possible transient store.

Before debugging, `deja "Poolrad slot A post-Up second printer return U1C view
offscreen object dirty extents 88 rows first unsupported writer"` was attempted
and timed out without a reusable result. Reused and checked the existing
[first-view placement](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[offscreen transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md), and
[one-row resident model](RESIDENT_464_2632_RUN014_COMPOSED.md). This capture
uses a fresh private replay; it does not borrow the one-row model's RAM.

## Image and instruction bounds

The ignored DOS v1.3 `START.EXE` is SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored saved slot A is SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Span | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
| Original `GAME.OVR` unit `0x3BD:+0198..+019D`, file `[0x2EA8F,0x2EA94)` | 5 / 1 | `a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe` |
| Loaded `2D14:0198..019D` | 5 / 1 | `0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4` |
| Original resident `+2632..+2A0D`, START `[0x6CAB,0x7086)` | 987 / uncounted path body | `ee35f98f513d5144e92e55e2afafbe176bba9ab65acc905ea151c19c5a9d713a` |
| Loaded `0C88:2632..2A0D` | 987 / uncounted path body | `f47503d6c3cdf12965eb9c1f33349d49c3ae8f8ed7d07682691a1a7de33ba4fc` |
| Original and loaded SYSTEM Move `+1685..+16A8`, START `[0xC2FB,0xC31E)` | 35 / 16 | `1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972` |
| Original mode-one tail `+28E0..+2A0D` | 301 / 113 | `6c94e57f546a7788df39d4b854595fe582ababb3af6b73456189209169d63afc` |
| Loaded mode-one tail `0C88:28E0..2A0D` | 301 / 113 | `92e07d39ba3cd4554cbbbaf7c63f406a7bdecdf194a8a6e440da0cecb8a54636` |
| Original and loaded SYSTEM FillChar `+16A8..+16BC`, START `[0xC31E,0xC332)` | 20 / uncounted here | `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7` |

Capstone 5.0.7 in 16-bit mode reached both short-span and mode-one end
addresses exactly.
The full resident span includes unvisited branches; no instruction count or
full-body parity is inferred for it.

## Same-run native boundary

The fresh private run loaded slot A at visible `(15,4,N; 10:50)`. Its RAM
confirmed party `(15,4,6)` and window-0 words `(0,5,10)`. Exactly one Up
reached `3582:3492` with party `(14,4,6)` and offscreen pointer
`DS:[4A20]=3DE4:0008`. The 14,135-byte offscreen object still had SHA-256
`489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1`.

At `2D14:0198`, before its far call to `0C88:2632`, the object had the same
hash and no dirty rows. `SS:SP=1D16:3EB0` held destination `3DE4:0008`,
source `14A0:260A`, mask `0x000B`, outer row count 43, 11 four-byte
elements per row, starting row 16, and horizontal index 2. The 640 KiB
entry RAM SHA-256 was
`84152b26b7314f2548cd08a437f6048ffacb2bf0f3148d4bf96842d5f36b0c79`.

A memory breakpoint on `3DE4:05BD`, the lowest offset that differs in the
direct call's final object, observed `0x80→0x00`. At that stop the guest was
at `131C:16A3`, just after SYSTEM Move's `REP MOVSB`; its `SS:SP` was
`1D16:3E78`, direct return `0C88:29CB`, and enclosing return
`2D14:019D`. Only two object bytes differed from the pre-call object at
this stop; two dirty rows were set. The private watched RAM and object
SHA-256 values were respectively
`718b0a7a39d1b5da91fd71b99746b8329289dfcd0abaed52e7f707fe02fd8bee`
and `705b7e47c63c77d100f1a392d37ba9b6ba974b17635c49c95ae200a64ce9337a`.
This is the first value-changing *watched address* in this replay. The
single-address watch cannot exclude earlier transient changes elsewhere.

The same run reached direct return `2D14:019D`. Its RAM SHA-256 was
`28306686e9d9516453b06a7614dc10ff1a15d587e878eecdd1ec97f496d83638`;
the object SHA-256 was
`ad329a4e8f7a2814ce07ef2e74cb225c731a6a9cd11e3edaac66723541517f0f`.
The call changed 546 of 14,135 offscreen bytes, none in the 23-byte header.
It marked exactly 43 dirty rows, 16 through 58. On all 43 rows, the recorded
left extent is 2, start extent is `(row*21+2)*4`, and end extent is start+43.
The first and last `(row,left,start,end)` tuples are `(16,2,1352,1395)` and
`(58,2,4880,4923)`; dirty flags outside this interval did not change.

## C page-state parity and boundary

The existing `pr_r464_2632_run014` C model explicitly requires the outer
row-count word at `[BP+0x10]` to equal 1. This reached frame supplies 43.
A strict private C probe fed the captured entry RAM plus the far-call return
frame to that model: it returned `PR_R464_2632_UNSUPPORTED_PATH` (numeric 2)
with all 1 MiB of caller-owned RAM unchanged. This is the correct boundary
for that older one-row model.

The new `pr_r464_2632_post_up_43row` parses the reached 18-byte caller
frame from `SS:SP`, requires the observed no-clipping shape and video state,
and rejects all other shapes before mutation. The original `+2910..+294D`
FillChar and four table/mask tests give the four-byte plane pattern from
caller-owned `DS:25F6..25F9` and mask `0x000B`. The original
`+294D..+2A07` loop marks each row dirty, conditionally adjusts both extent
bounds, and copies the pattern through SYSTEM Move. The C model derives all
output bytes from the input RAM; no native output tuple is embedded in it.

A strict C probe replayed the **same-run** `2D14:0198` entry RAM and compared
its output with the direct `2D14:019D` native return:

| Compared state | Exact bytes |
| --- | ---: |
| Entire offscreen object, including header | 14,135 / 14,135 |
| Dirty flags and three extent arrays `DS:4A28..4EBF` | 1,176 / 1,176 |
| Loop counter `DS:70C7` | 1 / 1 |

The model and native return differ in 55 bytes, all inside the resident and
runtime stack interval `SS:3E60..3EC1`. The private probe rejects a
difference anywhere else in the 640 KiB RAM image. Its changed-object count
is derived from the native entry/return pair and agrees at 546; this is a
value-change count, not a native write count. The model has no CPU or stack
parity and does not earn a full-C routine count. No later tile or flush
effect is composed here.

The focused synthetic test checks the plane mask, full 43-row bounds,
conditional retention of wider extents, and unsupported input with no RAM
mutation. It passed 1/1 under `-Wall -Wextra -Wpedantic -Werror`; the complete
strict CMake/CTest suite passed **246/246**. Public hygiene passed: no tracked
media or resources, copied research snapshot hashes, oracle shell syntax, and
Git whitespace checks. The private comparison command is:

```sh
resources/build-post-up-43row-p9/poolrad_resident_464_2632_post_up_43row_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/pre-0198-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/post-0198-second-ram-private.bin
```

The native recapture uses `BP 3582:3492`, sends exactly one Up, then uses
`BP 2D14:0198`, `BPM 3DE4:05BD`, and `BP 2D14:019D` in sequence, with
`MEMDUMPBIN 0000 0000 A0000` at each stop. Private RAM, media, emulator
trace, and captures remain ignored under `resources/`.
