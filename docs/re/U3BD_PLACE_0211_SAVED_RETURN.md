# U3BD +0211 saved return with supplied tile-callee effects

This bounded replay continues the [complete caller C candidate](U3BD_PLACE_0211.md)
through two tile calls in one saved slot-A native run. The tile callee itself
is **supplied** as each captured `+0300` post-call RAM and CPU14 state. The
checker compares the C caller's state at both `+02FB` callback boundaries,
then compares the completed C caller at the immediate `+0863` return.

The owner-supplied `GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The exact 270-byte caller at file `[0x2EB08,0x2EC16)` has SHA-256
`b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
The saved loaded body differs only in the tile-call segment relocation
`0x0428 -> 0x0C4C`.

The six private 640 KiB snapshots, in order, are `ram-0860-precall.bin`,
`ram-02fb-first-pre.bin`, `ram-0300-first-post.bin`,
`ram-02fb-second-pre.bin`, `ram-0300-second-post.bin`, and
`ram-0863-placement-return.bin`. Their hashes are pinned in the checker;
their CPU14 words are from the same-run debugger transcript. The checker
models the near-call return push before entering C. At each callback it
compares every captured low-RAM byte after adding the four far-return bytes
that the C model has already pushed, and compares all 14 CPU words. It then
copies the captured post-callee low RAM and CPU14 into the callback state.
The final comparison covers all 640 KiB and CPU14 at `+0863`.

Run with private captures under ignored `resources/`:

```sh
python3 tools/replay/u3bd_place_0211_saved_return.py \
  --original /path/to/ignored/GAME.OVR \
  --capture-dir /path/to/ignored/slot-a-u3bd-art-effect-p5-032
```

This validates caller/callback composition **conditional on the captured
tile-callee returns**. It does not independently replay the real tile callee,
compare the upper 384 KiB of natural RAM, establish a general gameplay route,
or add a strict full-C identity/count credit. The existing synthetic
original-byte differential is the full-1MiB caller test under its explicit
no-effect callback. A bounded `deja` query for the tile-call ABI timed out;
the implementation reuses the versioned [caller note](U3BD_PLACE_0211.md)
and [first-wall handoff](SLOT_A_U3BD_FIRST_WALL_TILE.md), with six private
captures checked independently by hash. No original media, RAM snapshot,
graphics object, or debugger transcript is tracked.
