# U3B0 entry4 `+0E0C`: reached non-null loop setup

`src/recovered/overlay_u3b0_entry4_loop_setup_0e0c.c` executes the
original DOS v1.3 `GAME.OVR` instructions at `+0E0C..+0E2F` and stops
before the first node-loop instruction at `+0E30`. This frame is
reached after both original SYSTEM `0AF8:0B25` copies, the taken
`+0DC7` branch, the `+0DEA` prefix, and the original `+0DFF` call to
the `+0067` node-count leaf. It is a bounded setup, not a complete
entry4 translation or a natural gameplay frame.

The pre-implementation `deja "Poolrad U3B0 entry4 +0E0C after node
count caller setup before +0E30 loop"` query found no prior session
match. The predecessor provenance reuses the reviewed `+0DEA` head
`63060f9c85d05e802121c9280cc648253eb6cfb7`, the `+0DFF`
caller at `f4c9e515f1d7408be2597e496a4f343bb98106cb`, and the
`+0067` leaf at `53c87573f3ead2b91ffa2e1590cedbcd60513146`.
Original bytes independently confirm each boundary. Local and remote
ownership scans found no U3B0 `+0E0C` source.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Setup | `[0x2D6DB,0x2D6FF)`; `+0E0C..+0E2F`; 36 bytes / 14 instructions; SHA-256 `68b2baacca017ff0e375559e8ee1bebf3cb0c51b2e9e31fd83238685b35f6409` |
| Caller | `[0x2D6CE,0x2D6DB)`; 13 bytes; SHA-256 `d23c4f4e4838df0b499375696ce48731a564594277605903bee06c3ee1b1fdf8` |
| Node-count leaf | `[0x2C936,0x2C97A)`; 68 bytes; SHA-256 `965144eab404415b7ce5d3f6e22d1c6991af4725dac559160302d89305468693` |

A controlled one-node case with incoming `SP=8000`, source string
lengths `5/9`, head `5000:2200`, and coordinate bytes `05/02`
reaches `+0E0C` after `+0DEA → +0DFF → +0E06 → +0067 → +00A8 →
+0E09`. The exact successor frame has `CS:IP=F000:0E0C`, `AX=0001`,
`BX=7E62`, `CX=0000`, `DX=0000`, `SI=110A`, `DI=2200`, `BP=7FFE`,
`SP=7E70`, `DS=6000`, `ES=5000`, `SS=7000`, flags `0246`, and
`SS:[BP-56]=0001`; full 1 MiB RAM SHA-256 is
`ad69167cfb2e8299059fc5d4e80a0f72154d1092cc6acfd8565e414151205a16`.
This is controlled reach only.

The C model sign-extends both coordinate bytes, stores their inclusive
width at `SS:[BP-59]`, copies the head pointer to local `BP-89/-87`,
clears the local count at `BP-8B`, and stops at `+0E30`. The oracle
executes the original predecessor bytes and compares native setup
versus C from the same `+0E0C` frame. **48/48** cases match every byte
of 1 MiB RAM and all 14 CPU words. Cases vary incoming SP, copy
lengths including the 40-byte cap, three non-null head shapes, and
positive/negative signed coordinate bytes. AF after `XOR` is
architecturally undefined; full-flags parity follows Unicorn's
original-byte convention without a native AF claim.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3B0 entry4 `+0E0C..+0E2F` setup before node loop |
| `source_candidates` | `pr_u3b0_entry4_loop_setup_0e0c` in `src/recovered/overlay_u3b0_entry4_loop_setup_0e0c.c` |
| `signals` | Exact 36-byte hash and 14 decoded instructions; two original SYSTEM returns; original node-count call and return; reached +0E0C frame |
| `score` | 48/48 original-byte full 1 MiB RAM/all14 CPU cases; focused strict C11 `-Werror` ASan/UBSan CTest |
| `verdict` | `match` for this bounded controlled setup; `unclear` for natural reach and complete entry4 identity |
| `review_notes` | Stops before `+0E30`; loop and later dependencies remain. No full-entry4 or strict full-C count credit. |

Reproduce from this branch's root with the persistent replay checkout:

```sh
cmake -S . -B resources/build-u3b0-0e0c \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3b0-0e0c --target poolrad_overlay_u3b0_entry4_loop_setup_0e0c_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3b0-0e0c \
  -R '^poolrad_overlay_u3b0_entry4_loop_setup_0e0c_tests$' --output-on-failure
REPLAY_ROOT=/path/to/poolrad-decomp-codex-replay-join
PYTHONPATH="$REPLAY_ROOT/resources/review/capstone5" \
  "$REPLAY_ROOT/resources/venv-string0356/bin/python" \
  tests/recovered/check_overlay_u3b0_entry4_loop_setup_0e0c.py
```

Original media and sanitizer builds remain under ignored `resources/`.
