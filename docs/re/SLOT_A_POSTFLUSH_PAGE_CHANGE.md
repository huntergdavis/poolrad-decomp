# Saved slot-A page after the second resident flush

An uninterrupted saved slot-A DOS run links the second
`RES_0464:1B52` flush return to the first-view return and the next
prompt-row drawer call. The full four-plane `A000` page stays equal over
the first three measured stops. The call at `2116:048B` to unit
`0x3B0:+01DA` then produces the first **persistent page difference at
these stops**: 518 changed EGA plane-byte values on its return at
`2116:048E`. The already merged bounded C prompt-row model matches the
32,000-byte native return page exactly. This identifies a reached page
effect, not the first instruction-level hardware write or a complete RGB
frame.

Before debugging, `deja "Poolrad saved slot A post second RES_0464 flush
earliest EGA plane value change palette metadata first frame"` was attempted with a
25-second bound and timed out. This audit reused the merged
[prompt-row model](SLOT_A_PREINPUT_PROMPT_ROW_PAGE.md), the
[call-42 same-run checkpoint](SLOT_A_POST42_PROMPT_ROW_SAME_RUN.md), and
the merged [second-flush audit](SLOT_A_RES464_SECOND_FLUSH.md) from PR #431. The private run is
`resources/oracle-runs/slot-a-postflush-plane-change-p5-001`. It does not
reuse another run's RAM or EGA images.

## Addresses and byte evidence

The ignored DOS v1.3 `START.EXE`, `GAME.OVR`, saved slot A, and heavy
DOSBox-X SHA-256 values are respectively:

- `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
- `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
- `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`
- `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`

The original resident flush span `START.EXE[0x61CB,0x64D5)` has
SHA-256 `7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
The original unit `0x3B0:+01DA..+037C` drawer span, 419 bytes, has
SHA-256 `8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce`.
The original and loaded three-byte near call at `+048B` both hash
`a47ae6e9d1136bdf24bdde8a222cffe3c2c5df0d98fec86529b0650bd2bdf4f3`;
its relative target is `+01DA`. The audit also checks the relocated
loaded drawer and flush spans.

The private debugger transcript SHA-256 is
`ca678e59b58d0dcd85644bd04fa396eb53cbe0fe00a2e82b48d6c63547a65c38`.
Its ordered observed PC sequence after the initial restore prompt is
`0C88:1B52` twice, `2D14:05FB`, `3582:3387`, `2116:048B`, and
`2116:048E`. At the second flush entry, `SS:SP=1D16:3EBE`; the
immediate return has `SS:SP=1D16:3EC2`. The second flush is therefore
distinguished from the earlier `SS:SP=1D16:3E94` call.

| Native stop | 640 KiB RAM SHA-256 | 32,000-byte EGA page SHA-256 |
| --- | --- | --- |
| second flush return `2D14:05FB` | `909bc77480f1dfac6f7307819ffb80eca55ecc2be1a2c227c038eb610b48e7d2` | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` |
| first-view return `3582:3387` | same | same |
| drawer entry `2116:048B` | `e592be642b4b2cb1fb2ee8dffa7af6f941861034feeab03debe4f4c803857061` | same |
| drawer return `2116:048E` | `ecefb60da63cb6281faa4eb4f9e5854fab87aff192cfb8ce9e5e9cfe3aa331c0` | `09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a` |

The return page differs at 518 values across planes `(63,196,63,196)`,
limited to scanlines 192..198 and byte columns 0..32. The private live
drawer frame has length 33, column 0, selected positions 1..4,
selection 1, and selected/ordinary colors 15/10. The same-run C probe
applies that frame, the hash-locked live font and classifier set, and the
measured entry page. It reports 39 cells and 1,248 modeled plane stores;
its **32,000/32,000 output bytes** equal the native drawer return.
Between drawer entry and return, 32 RAM bytes change, all within the
stack segment. Party, window-0 minute fields, ECL PC, and selection are
unchanged at all four captured stops.

## Reproduction and limits

```sh
cmake -S . -B resources/build-slot-a-postflush-change-p5 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-postflush-change-p5 --target \
  poolrad_slot_a_u3b0_prompt_row_tests poolrad_slot_a_u3b0_prompt_row_probe
ctest --test-dir resources/build-slot-a-postflush-change-p5 \
  -R '^poolrad_slot_a_u3b0_prompt_row_tests$' --output-on-failure
python3 tools/replay/slot_a_postflush_page_change_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --game resources/DOS/POOLRAD/GAME.OVR \
  --save resources/DOS/POOLRAD/SAVGAMA.DAT \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-postflush-plane-change-p5-001 \
  --probe resources/build-slot-a-postflush-change-p5/poolrad_slot_a_u3b0_prompt_row_probe
```

The focused strict synthetic CTest passed 1/1. The audit hash-locks the
original inputs, ordered native transcript, four full RAM stops, four
complete EGA pages, loaded code, live model inputs, and C output. Page
snapshots do not exclude transient writes that were later restored or
stores of the same value between stops. Palette/register changes and a
whole visible RGB frame are separate boundaries; display ownership
should be coordinated before that join. No full-routine or identity
count follows from this page-effect bridge. Original media, extracted
bytes, RAM, planes, and screenshots stay under ignored `resources/`.
