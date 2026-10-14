# Saved slot-A UI: post-drawer no-key wait page

This bounded native trace follows the saved-slot prompt drawer return at
`2116:048E` through the first wait at `2116:0566`, one zero-result key poll,
and the next wait at `2116:0566`. The full four-plane EGA page is unchanged at
all three stops: **32,000/32,000 bytes equal**. The reached no-key path has no
value-changing page writer. This is a measured waiting interval, not a claim
about the subsequent key route, a complete UI frame, or a C game-function
count. A page-effect C model is deferred until a changed page is measured.

Before analysis, `deja "Poolrad slot A 2116:048E prompt drawer return to
2116:0566 waiting input first EGA page write cursor blink"` returned no
usable result within 30 seconds. The existing [gameplay path](GAMEPLAY_PATH.md)
identified the `+048E..+06D5` input loop, and the merged prompt-row evidence
from PR #393 supplied the first-view and drawer anchors. This fresh run checks
the post-drawer path independently.

| Ignored input | SHA-256 |
| --- | --- |
| DOS v1.3 START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A save | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The private run is `resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094`.
After the visible slot-A selection, the first-view return was observed at
`3582:3387`. The next prompt drawer return was `2116:048E`; the first and
second waiting stops were `2116:0566`. All three captured pages have SHA-256
`09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a`.

| Stop | 640 KiB RAM SHA-256 | Page bytes changed from prior stop |
| --- | --- | ---: |
| Drawer return `2116:048E` | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` | reference |
| First wait `2116:0566` | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` | 0 |
| Second wait `2116:0566` | `4ee56e79a63b3c8991afeb5c83a90d768bb92c758939a1af4eb8da5cc784b6c8` | 0 |

The live far pointer `[DS:49D2]` was `3996:0000`; its window-0 word `+03FE`
was 3. Party `(DS:6AAD..6AAF)` was `(15,4,6)`, window-0 time words
`+018E/+0190/+0192` were `(0,5,10)`, and map byte `DS:84DC` was 20.
`DS:6ABF`, `DS:6ABE`, words `DS:6B2E` and `DS:6B30`, and local
`SS:[BP-63]` were zero at the captured stops. The resident poll returned
`AL=0` at `2116:056B`; the `+06CF` stop showed local `-63=0` and the
`+06D5` jump back to `+048E`. Across that complete loop, only ten stack RAM
bytes at physical `0x20F5E..0x20F67` differed.

| Original bounded span | Length | SHA-256 | Loaded-byte check |
| --- | ---: | --- | --- |
| GAME.OVR unit `0x3B0:+048E..+0565` | 216 | `1709d5b2bec86d104ce166c83b7b858828dd28b6794bbebe2ed95881c3c858ab` | Only five far-call segment words relocated by `+0x824` |
| GAME.OVR unit `0x3B0:+0566..+0571` | 12 | `cc4b28d4950373552e6630c9271f61a579ce77c835376ae4b43f5a247effb386` | One far-call segment word relocated by `+0x824` |
| GAME.OVR unit `0x3B0:+06CF..+06D7` | 9 | `c24571c84c5a3cb61e27a19efc23ad8695b66634c428cae065cb3bad1a954916` | Exact loaded match |
| START.EXE resident `0x0A40:+02FD..+030E`, file `0xA3F6..0xA407` | 18 | `83af1ee6b1d5cebb5dee97b742228b8a4fb3cbb12ade84b39e5ce82275e7fc5e` | Exact loaded match at `1264:02FD` |

The reached path from `+048E` takes the nonzero window test to `+04A3`;
`DS:6ABF=0` jumps to `+0530`; zero `DS:6B30/6B2E` branches to `+0566`.
Those instructions contain no call or EGA store. The 18-byte resident poll
checks its flag, calls BIOS keyboard status (`INT 16h`, `AH=1`), returns
`AL=0`, and does not store to EGA memory. The caller's zero-result branch
goes `+056B` → `+056F` → `+06CF` → `+06D5` → `+048E`. The nonzero key path
from `+0572` is outside this trace and belongs to the key-route lane.
Equal page snapshots alone would not rule out transient writes; the reached
branch and poll bytes bound this specific no-key route.

Reproduce the private audit with the ignored hash-pinned inputs:

```sh
python3 tools/replay/slot_a_postdrawer_wait_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --game resources/DOS/POOLRAD/GAME.OVR \
  --save resources/DOS/POOLRAD/SAVGAMA.DAT \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094
```

The replay was prepared with `tools/oracle/prepare_run.sh` and launched with
the heavy debugger through `tools/oracle/launch.sh`. At the visible slot
chooser, arm `BP 3582:3387` and select A. At that stop, arm
`BP 2116:048E` and `BP 2116:0566`. At each stop, dump RAM with
`MEMDUMPBIN 0000 0000 A0000`; select EGA read maps using `OUTP 3CE 04` and
`OUTP 3CF 00` through `03`, then dump each plane with
`MEMDUMPBIN A000 0000 1F40`. Restore read map 0 and selector 5 before
resuming. At the first `+0566`, also stop at `+056B` and `+06CF` to verify
the zero-result branch; then continue to the second `+0566`. Screenshots,
save, media, RAM, EGA pages, and emulator output remain under ignored
`resources/`.

The strict CMake build and full CTest suite passed **189/189** on main
`75d24676525ad0a984e66cd14764de87638d96f2`; the public repository
hygiene gate passed. The private audit above passed on that same tree.
