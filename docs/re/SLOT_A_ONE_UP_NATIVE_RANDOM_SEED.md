# Saved slot-A Up: live ECL RANDOM seed and store

This note records the ECL RANDOM instruction reached by exactly one native DOS
Up from the hash-pinned slot-A save. It measures the 32-bit resident seed at
`DS:4428..442B`, the range and result of the reached RNG call, and the actual
window-2 destination word. It is a native observation, not a C-native replay or
an end-to-end parity claim. This is the ECL RANDOM handler in unit `0x1C`; the
separate unit `0x9E` dice helper is outside this observation.

## Inputs and address proof

The prior [party-input audit](SLOT_A_PARTY_INPUT.md) identified opcode `0x08`
at saved script PC `0x9B3A`, and the [bounded handler map](U1C_RANDOM_0234.md)
established unit `0x1C:+0234..+0291` as a 94-byte, 35-instruction body. The
prior native [resident-dispatch replay](SLOT_A_ONE_UP_NATIVE_RESIDENT_DISPATCH.md)
supplied the single-Up preparation procedure. A fresh `deja` lookup for this
specific ECL RANDOM address produced no result before timing out at 12 seconds;
the earlier broad lookup was cancelled after it stalled. The linked checked-in
notes are the prior evidence reused here.

| Ignored private input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

`GAME.OVR` file `0x0F7A..0x0FD7` hashes
`a634c69e97fe005c2041e9d18d0874d0eb8ede7f4a59833bfb642918b7c5d979`.
The corresponding 94 runtime bytes at `3582:0234..0291` hash
`a593b691aa2a4c01692603367a4647e8d32d90e9518f3647edfbe7538354372b`;
the difference is loader relocation. At the reached handler entry,
`DS:[49ED]=0x9B3A`, `DI=0x9B3A`, and party bytes `DS:6AAD..6AAF` are
`(14,4,6)`. The saved view before the one Up showed `(15,4,6)` at 10:50.

At runtime, the four far calls in this handler target the following guest
addresses. Each five-byte call site is hash-locked from the private pre-call
RAM dump; no instruction bytes are published.

| Live call site | Target | Five-byte SHA-256 |
| --- | --- | --- |
| `3582:026C` | `131C:11C0` | `bb2d17a6b84acad7af12148001b4d5ddbe767c2c2f3559b50bfa17053968870d` |
| `3582:0273` | `131C:119A` | `bfa1907c34a7f3352e37bd218563496f8c63a4cd02be457ce386a570cae16527` |
| `3582:0278` | `131C:119E` | `06539c6dc23b3c7b40c0c7a444c13cc986329d3d203b43dde1aebef23a9cae19` |
| `3582:0289` | `084F:0052` | `4f3457fbcbfa0893d58d8fadc77d12a070bc02a46b0dab4dd9b842faab5f2720` |

`131C` is the live resident segment corresponding to source segment `0x0AF8`.
The reached nested generator entry was `131C:120F`.

## Measured stop sequence

`DS=14A0`, `SS=1D16`, `BP=3EE2` at the handler stops. The window-2 far
pointer `DS:[49DA]` was `3A96:0000`; script operand `0x980F` maps to the word
at this window's `+021E`. The table reports that word, not a guessed script
result. Seed values are unsigned 32-bit decimals.

| Stop | Seed at `DS:4428` | Range/result and destination |
| --- | ---: | --- |
| `3582:0234` | 3140872353 | Entry; destination word 16. |
| `3582:026C` | 3140872353 | Before `131C:11C0`: pushed range 14; `AX=14`, `SS:SP=1D16:3EDC`; destination 16. |
| `131C:11C0`, `131C:120F` | — | RNG entry and nested generator entry reached. |
| `3582:0271` | 3238563622 | RNG returned `AX=10`, `AL=10`; destination still 16. |
| `3582:0273` | 3238563622 | After `xor dx,dx`, `AX=10`, `DX=0`; call `131C:119A` follows. |
| `3582:0278` | 3238563622 | After `119A`, `AX=132`, `CX=5`, `DX=8192`; call `131C:119E` follows. |
| `3582:027D` | 3238563622 | After `119E`, `AX=10`, `AL=10`; destination still 16. |
| `3582:0280` | 3238563622 | Local low result byte `[BP-2]=10`; destination still 16. |
| `3582:028E` | 3238563622 | After `084F:0052` word store, destination **10**. |

The measured seed transition equals
`(3140872353 * 0x08088405 + 1) mod 2^32 = 3238563622`, and
`(3238563622 >> 16) mod 14 = 10`. These arithmetic checks agree with the
earlier resident decode; the return registers and destination above are live
observations. No later script branch or C result is inferred here.

## Private replay checkpoints

The save, RAM dumps, screenshots, and emulator trace remain ignored under
`resources/oracle-runs/slot-a-random-live-006/`. Each snapshot is a 640 KiB
guest RAM dump; only its SHA-256 is recorded here.

| Stop | RAM SHA-256 |
| --- | --- |
| `3582:0234` | `7f6299fc402cac8e2c3a00d9583263742bad049efc5b4a70440763a9bd310e44` |
| `3582:026C` | `140f986987ecd8dee07d3b31a9f94cb0967baba498eb1ec60d362ccfb6854aec` |
| `3582:0271` | `5926aee8d360d31b5f0be15b1d9696123f0d96828228b137279125a659c9fb18` |
| `3582:027D` | `e66d234f2baa506aca6d45c962c264c75a84ff94aaf9634e627a64d40fa7d1ed` |
| `3582:0280` | `dda0ae17eeccaad05fd4c0c4e332bf64b1e3486a95a539346648c038e58e836c` |
| `3582:028E` | `534019e1955933528d114129459dbddfe95bce9e4f35b8a0c63ed11f57cc9097` |

Reproduce from the same ignored inputs in a private working copy:

```sh
tools/oracle/prepare_run.sh slot-a-random-live-006
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a tools/oracle/launch.sh resources/oracle-runs/slot-a-random-live-006 \
  -time-limit 1200 -c "AUTOTYPE -w 160 -p 8 enter l , a"
```

At the playable saved view, pause with Alt+Pause, arm `BP 3582:0234`,
`BP 3582:026C`, `BP 3582:0271`, `BP 3582:0273`, `BP 3582:0278`,
`BP 3582:027D`, `BP 3582:0280`, and `BP 3582:028E`, continue with F5,
then send exactly one Up. `EV AX BX CX DX SI DI CS IP DS SS SP BP` and
`MEMDUMPBIN 0000 0000 A0000` provide the private samples. The guest was
stopped after the store. No external network or GitHub action was used.
