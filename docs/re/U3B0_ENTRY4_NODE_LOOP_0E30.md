# U3B0 entry4 `+0E30`: reached finite node-flag loop

`src/recovered/overlay_u3b0_entry4_node_loop_0e30.c` executes the
original `+0E30..+0E5E` loop and stops before `+0E5F`. Its controlled
input frame follows both original SYSTEM `0AF8:0B25` copies, the
non-null `+0DC7` branch, the `+0DEA` prefix, the original `+0DFF`
call to the `+0067` node-count leaf, and the reviewed `+0E0C` setup.
No copy output or node-count result is fabricated. This is a bounded
loop, not a complete entry4 translation or natural gameplay reach.

The target-specific `deja "Poolrad U3B0 entry4 +0E30 node flag loop
+0E5F original bytes parity"` query timed out (exit 124) and
supplied no reusable claim. The predecessor frame and setup are from
the reviewed `+0E0C` head
`bbfda2049d48cc07752c471f5b3925d7297a747a`, with the original
`+0DFF` caller and `+0067` leaf re-executed here. The local and
remote ownership scan found no other U3B0 `+0E30` source.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Node loop | `[0x2D6FF,0x2D72E)`; `+0E30..+0E5E`; 47 bytes / 13 instructions; SHA-256 `1f47b45e511eb13f59a8c73c7ef6f16c956975def923bbbd56eaaf89d7023407` |
| Exit | `JE +0E5F` for null pointer or zero node flag at `ES:[DI+29]` |
| Traversal | Reads next far pointer at `ES:[DI+2A/+2C]`, stores it at `SS:[BP-89/-87]`, increments `SS:[BP-8B]`, jumps to `+0E30` |

In a controlled one-node case with head `5000:2200` and node flag
zero, the original path from `+0E0C` enters `+0E30`, tests the node
byte, and stops at `+0E5F`. The resulting CPU has `CS:IP=F000:0E5F`,
`AX=7200`, `DX=5000`, `DI=2200`, `ES=5000`, `BP=7FFE`, `SP=7E70`,
flags `0246`; full 1 MiB RAM SHA-256 is
`61360faefb51ae6a48ff0335a1eca8ecde3a5e595ca4d0977199c4a78a1abd12`.
This is an exact controlled frame, not a saved-game frame.

The oracle seeds finite node chains before the original `+0067`
caller reads them, and verifies the leaf's returned count. It then
compares original loop bytes and C from the same reached `+0E30`
frame. **56/56** cases match all 1 MiB RAM bytes and all 14 CPU words
at `+0E5F`. Cases cover 1–3 nodes, zero and nonzero flags (including
`0x80`), zero-flag and null-link exits, two incoming stack positions,
two Pascal copy-length pairs, and two signed coordinate pairs.
The C API bounds the number of traversals; a cycle beyond the caller's
limit returns `LIMIT` at the first unexecuted `+0E45` instruction.
No termination claim is made for cyclic original input. AF after `OR`
is architecturally undefined; full-flags parity follows Unicorn's
original-byte convention without a native AF claim.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3B0 entry4 `+0E30..+0E5E` finite node-flag loop |
| `source_candidates` | `pr_u3b0_entry4_node_loop_0e30` in `src/recovered/overlay_u3b0_entry4_node_loop_0e30.c` |
| `signals` | Exact 47-byte hash and 13 decoded instructions; two original SYSTEM returns; original node-count call and finite chain trace |
| `score` | 56/56 original-byte full 1 MiB RAM/all14 CPU cases; focused strict C11 `-Werror` ASan/UBSan CTest |
| `verdict` | `match` for finite controlled chains to `+0E5F`; `unclear` for natural reach and complete entry4 identity |
| `review_notes` | Stops before `+0E5F`; later node loop, calls, and return remain. No full-entry4 or strict full-C count credit. |

Reproduce from this branch's root with the persistent replay checkout:

```sh
cmake -S . -B resources/build-u3b0-0e30 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3b0-0e30 --target poolrad_overlay_u3b0_entry4_node_loop_0e30_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3b0-0e30 \
  -R '^poolrad_overlay_u3b0_entry4_node_loop_0e30_tests$' --output-on-failure
REPLAY_ROOT=/path/to/poolrad-decomp-codex-replay-join
PYTHONPATH="$REPLAY_ROOT/resources/review/capstone5" \
  "$REPLAY_ROOT/resources/venv-string0356/bin/python" \
  tests/recovered/check_overlay_u3b0_entry4_node_loop_0e30.py
```

Original media and sanitizer builds remain under ignored `resources/`.
