# Saved slot-A U3BD entry-5 `+080A` branch and next call

This bounded model starts at the reached `2D14:+080A JNE` after the local
leaf call documented in PR #397. For the saved first-view route, it reaches
the `+0860` callsite and stops at the entry of local `+0211` after the near
call pushes its return IP. It does **not** execute the `+0211` callee or
claim a complete entry-5 or visible-frame translation. It adds no full-C
or identity count.

Before decoding, `deja "Poolrad U3BD entry5 +080A JNE +0824 next callee
saved slot A"` timed out at the 30-second bound without a result. The
merged [first-call handoff](SLOT_A_U3BD_FIRST_CALL_07E9.md) establishes
the preceding saved route. Merged PR #397's `+07F9..+080A` local-leaf evidence
was reused, and the new same-run `+080A` frame independently
matches its manually observed CPU values. The `+080A` instruction and all
later bounds below were decoded afresh from the hash-verified original.

## Original-byte boundary and path

The ignored DOS `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The caller span `+080A..+0863` is file `[0x2F101,0x2F15A)`, 89 bytes and
41 instructions, SHA-256
`623e419028f0b9f65c3db7716c85912668fea14f8d4099eceb9210343908263b`.
The entire span matches loaded `2D14` memory at the first native stop.

| Address | Reached saved-route effect |
| --- | --- |
| `+080A` | `JNE +0824` is taken because the preceding leaf result was nonzero. |
| `+0824/+0828` | Compare local `SS:[BP-14]` with zero; the saved value is nonzero, so `JE +0865` is not taken. |
| `+082A/+082E` | Compare local `SS:[BP-17]` with zero; the saved value is zero, so `JBE +0847` is taken. |
| `+0847..+085F` | Copy local `BP-14` to `BP-17`; add the `DS:0CAA` word to local `BP-12`; push that result, two byte-derived words and one immediate word with preserved `AH`, then `CS`. |
| `+0860` | Near-call local `+0211`, pushing return IP `+0863`. The model stops before any callee instruction. |

The opposite `+080A` arm stops at its `+047B` local call entry. The
nonzero previous-tile arm stops at its first `+0211` call entry at `+0844`.
When local `BP-14` is zero, the model stops at branch target `+0865`.
These are bounded transfers; no later callback result is inferred.

## Same-run native and C gates

Private run `slot-a-u3bd-jne-080a-p5-028` was prepared with the hash-locked
oracle copy and isolated debug DOSBox-X (binary SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`).
The saved slot-A input SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Three debugger stops, their EV output, and three full conventional-RAM
images were retained in one ignored run. The 10,865-byte private debugger
transcript SHA-256 is
`27de497745a0e8f588f0a436acae267e9c15585c6695f279aa3719680d0a7885`;
the audit parses exactly 42 field/value EV pairs and three successful dump
records from that transcript.

| Stop | Private 640 KiB RAM SHA-256 | Changed bytes from previous stop |
| --- | --- | ---: |
| `+080A`, before JNE | `0d06b515e83e796ade8e41d3c93e3c999a43bcd291bb0efaebd2ee58ab74842a` | — |
| `+0860`, before CALL | `0d86321fa96c9a0bc11fedd625d15f483ed838c53062cbfea44c317ad7cc658a` | 8 stack/local bytes |
| `+0211`, callee entry | `1c4354e8a6ad27344b98d7615b4ccce6b37f055f5a5c403d745d09d28b72464b` | 2 return-IP bytes |

The pre-call stack contains five pushed words; the callee-entry stack
adds return IP `+0863`. `tools/replay/u3bd_jne_080a_audit.py` checks the
original and loaded span hashes, transcript hash and CPU stop addresses,
all three RAM hashes, and the exact changed-address sets. It runs C from
the first native image twice: stopping before the call and after its
return-IP push. Both outputs match the corresponding native image across
all 640 KiB, all 13 CPU fields, and IP. Original game files, RAM, screenshots,
and the EV transcript remain ignored under `resources/`.

## Reproduce

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/replay/u3bd_jne_080a_audit.py
cmake -S . -B resources/build-u3bd-jne-080a -DCMAKE_C_FLAGS='-Wall -Wextra -Werror -pedantic'
cmake --build resources/build-u3bd-jne-080a --target poolrad_overlay_u3bd_jne_080a_tests
ctest --test-dir resources/build-u3bd-jne-080a -R '^poolrad_overlay_u3bd_jne_080a_tests$' --output-on-failure
```

The synthetic CTest covers both saved stop modes, the opposite JNE call,
zero and nonzero local branches, argument/return stack words, defined
arithmetic flags, and invalid host inputs. The next unmodeled behavior is
inside local `+0211` after the saved call handoff.
