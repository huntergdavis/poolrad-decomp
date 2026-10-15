# Saved slot-A post-Up row-59 resident call at `2D14:01BE`

**Classification:** reached direct-call frame with exact C/native return parity
for the existing bounded one-row resident model. This is a second invocation
of `0C88:2632`, not a new complete translation or a full static frame.

Before this investigation, `deja "Poolrad saved slot A exactly one Up U3BD
2D14 01BE 01C3 RES_0464 2632 one row after 43 row offscreen native frame"`
was attempted with a 25-second bound and returned no reusable result. The
earlier [43-row post-Up gate](SLOT_A_POST_UP_OFFSCREEN_43ROW_GATE.md) and
[one-row resident model](RESIDENT_464_2632_RUN014_COMPOSED.md) were then
rechecked against a fresh direct call in the same hash-pinned saved replay.

## Image, call, and frame

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

The original `GAME.OVR` unit `0x3BD:+01BE..+01C3`, file
`[0x2EAB5,0x2EABA)`, is one five-byte far-call instruction, SHA-256
`a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe`.
Its encoded original target is `0464:2632`. At loaded `2D14:01BE..01C3`
the one-instruction span SHA-256 is
`0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4`,
and its relocated target is `0C88:2632`. The original resident
`+2632..+2A0D` is 987 bytes, SHA-256
`ee35f98f513d5144e92e55e2afafbe176bba9ab65acc905ea151c19c5a9d713a`.
The resident body contains other paths, so its full instruction count and
complete semantics are not claimed here.

A private DOSBox-X replay loaded slot A at visible `(15,4,N; 10:50)`, sent
exactly one Up, and stopped at `2D14:01BE`, then at resident `0C88:2632`,
then at direct return `2D14:01C3`. At the caller, `SS:SP=1D16:3EB0` held
destination `3DE4:0008`, source `14A0:260A`, mask `0x000F`, outer row
count 1, element count 11, row 59, and x 2. The resident entry had
`SS:SP=1D16:3EAC` and reaches local `BP=3EAA`; its return frame points to
`2D14:01C3`. The earlier direct call at `2D14:0198` had already marked
rows 16–58 dirty. The `01BE` call is separately observed, rather than
inferred from that first call.

## Native result and C boundary

The 640 KiB RAM SHA-256 values at caller, resident entry, and return were,
respectively,
`83f7a493a86eef7dde98da79c288ebbd7b9675639f397caf4234d3bc26558528`,
`32564707af1b321225cf6676f80c5c24cd7f070517f0d1a7565b0fd876df3023`,
and
`29d3efb8fdfd6c9d363b9b0ac7ead4d6844713fdd5e1d7b40b32247172846870`.
The 14,135-byte offscreen object SHA-256 changed from
`ad329a4e8f7a2814ce07ef2e74cb225c731a6a9cd11e3edaac66723541517f0f`
to
`6a9b03b96c33f3cc3dab737ffb94e28d9b87eff8993d2678cbed0c4f863ce132`.
The entry-to-return pair differs at 25 RAM bytes, 13 inside the object.
Dirty rows increase from 43 (16–58) to 44 (16–59). Row 59 has left
extent 2, start 4964, end 5007; the loop byte `DS:70C7` is 11.
These are value-change counts, not write counts.

The already merged `pr_r464_2632_run014` model accepts exactly this
one-row, positive-x, mode-one frame. Given the *new* captured resident-entry
RAM and measured 14-field CPU state, it returned `PR_R464_2632_OK`, then
matched the direct native return in **655,360/655,360 RAM bytes and 14/14
CPU fields**. The result derives from entry RAM and caller arguments; no
native output tuple or game bytes are compiled into the model. The focused
synthetic test seeds the preceding 43 dirty rows, checks the row-59 cell and
extent updates, and rejects a 43-row frame with no mutation. A separate
private mode repeats the exact native pair comparison:

```sh
resources/build-post-up-01be-p9/poolrad_resident_464_2632_post_up_01be_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/01be-resident-entry-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/01be-resident-return-ram-private.bin
```

The native replay uses `BP 2D14:01BE`, `BP 0C88:2632`, and
`BP 2D14:01C3`, with `MEMDUMPBIN 0000 0000 A0000` at each stop. This
gate ends at `01C3`. The later `02FB` tile handoff has not been measured in
this replay. Private RAM, save, media, emulator trace, and captures remain
ignored under `resources/`.
