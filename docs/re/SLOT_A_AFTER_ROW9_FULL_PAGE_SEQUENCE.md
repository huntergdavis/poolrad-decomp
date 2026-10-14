# Saved slot-A first view: next three printer calls after row nine

After the three verified [row-nine calls](SLOT_A_ROW9_THREE_CALL_COMPOSITION.md),
the saved slot-A native replay reaches three more `RES_0709:0626` printer
calls before the first-view return. Their return addresses, in observed
order, are `08DE:2B5A`, `08DE:07A1`, and `08DE:07BF`. Starting with the
same-run page at the row-nine third-call return, three sequential
applications of the existing bounded C printer-page model match **all
32,000 bytes** of each immediate native return page. The third C page
also matches the same-run `3582:3387` first-view EGA page over
**32,000/32,000 bytes**. This is parity for this page sequence, not a
complete rendered frame or a new full-routine translation.

Before tracing, `deja "Poolrad slot A first view next printer after row
nine 08DE:09C3 multi-call sequence"` found no matching session. The
merged [row-nine composition](SLOT_A_ROW9_THREE_CALL_COMPOSITION.md)
provided the starting stop and page hash; the full-plane replay below
independently checks the new calls, source inputs, and output pages.

## Original and loaded call proof

The ignored run is
`resources/oracle-runs/slot-a-ui-after-row9-full-p9-020/`, prepared
with `tools/oracle/prepare_run.sh`. DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
the private heavy DOSBox-X binary is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The audit hash-locks eight 640 KiB RAM captures and every 8,000-byte
plane at those stops.

The original `RES_00BA` call sites at `2B55`, `079C`, and `07BA` are at
expanded image offsets `0x36F5`, `0x133C`, and `0x135A`. Each five-byte
far call has SHA-256
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`
and target `0709:0626`. Each loaded call has SHA-256
`cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585`
and target `0F2D:0626`. The original bounded printer body
`[0x76B6,0x7737)` is 129 bytes, SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
the loaded `0F2D:0626..06A7` body hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.

| Call site; return | Original 32-byte context SHA-256 | Loaded 32-byte context SHA-256 | SS:SP | Source length; SHA-256 of length plus content | Low-byte color, row, column |
| --- | --- | --- | --- | --- | --- |
| `2B55`; `2B5A` | `b65227edc0424b619b4d3dc4da629eabd7bf5464662226210627149bb952bcd1` | `bb20ebcb0856a92678d7f14c88e7cfc162cb497048fd525dafb4c4fc81de0d8b` | `1D16:3C86` | 12; `28208170efa836a86605cb5e33e47e95c8717d0263668fd6428fff6c1a07c856` | 10, 15, 17 |
| `079C`; `07A1` | `605ed6ad6c8916c8f483d235a36f2c402714c969f84e4886f625ebec98a8ebe6` | `27cc354dd8c656fd9d7836c54a1b4f03c855db30b253689e94ac6df1602611de` | `1D16:3EB0` | 4; `4c1496b703656034f30aee34e197eb1f4abeff239d28dc9263af7a3df199090b` | 15, 2, 17 |
| `07BA`; `07BF` | `6b8659d17825890f46a38ee4490df31c6ae55b31be5ddd562c31b16b3836c0a2` | `737bdce626e86def418f7ba427d7d85373051a7023453edbff30e03e874d8723` | `1D16:3EB0` | 6; `7039295c05fc9db0246086bb5dc3046e4ac8edffb2c6376a760c88f229dc7ef0` | 15, 2, 33 |

At each printer entry, the seven stack words contain return CS:IP,
private source far pointer, then color, row, and column words. The
first call's last three words are `A00A,A00F,A011`; the next two are
`A00F,A002,A011` and `A00F,A002,A021`. The printer uses their low
bytes for color and position. All three entries retain an upper
`08DE:2894` caller return at `SS:3EC8`, live font SHA-256
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`,
`DS:4A1E=1` EGA mode, and both page segments `A000`. The row-nine
anchor frame has row 9, column 38, and return `08DE:08BA`.

## Same-run full-page result

Each page concatenates four 8,000-byte EGA planes from
`A000:[0x0000,0x1F40)`. The earlier 3,520-byte-per-plane window began
at `A000:03C0` and stopped before row 15; it therefore missed the
first new call's effect. Its row-nine starting slice hashes to
`d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b`,
matching the merged row-nine evidence.

| Native stop | 32,000-byte page SHA-256 | Net changed values from prior entry |
| --- | --- | ---: |
| Row-nine third return and call 1 entry | `c7ad8cf330e3c12800d6c72e68a274664efe6703031ac51ffe399ae5190096ce` | 0 |
| Call 1 return and call 2 entry | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` | 126 |
| Call 2 return and call 3 entry | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` | 0 |
| Call 3 return and `3582:3387` first-view return | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` | 0 |

The 126 immediate changed values are in planes 1 and 3 at global
scanlines 120–127 and columns 17–28. The two row-two calls issue
modeled stores but leave the observed page values unchanged. Across
the three calls, the C model reports **22 cells, 704 plane stores, and
126 changed values**: `12/384/126`, `4/128/0`, and `6/192/0`.
Each C output matches its immediate native return over
**32,000/32,000 bytes**; the final C output also matches the same-run
first-view page over **32,000/32,000 bytes**. Snapshot equality cannot
exclude same-value or transient native writes. The public synthetic
test checks the full four-plane page after each invented call, including
pre-existing row-two glyphs and untouched bytes.

## Reproduce locally

Prepare a fresh ignored run and start the pinned heavy DOSBox-X with
`DEBUGBOX START`. At the title press Return; if the codeword prompt
appears, press Return there; at the main menu press `l`, then at
`LOAD WHICH GAME` press `a`. Arm `BP 08DE:09C3`: its fifth hit is
the prior row-eight column-38 return, and its sixth hit is the
row-nine column-38 return. Capture the sixth hit and then the next
three `0F2D:0626` printer entries and returns `08DE:2B5A`,
`08DE:07A1`, and `08DE:07BF`. Then capture `3582:3387` first-view
return. At each stop, `MEMDUMPBIN 0000 0000 A0000` captures private
RAM. For each EGA plane 0–3, use `OUTP 3CE 04`, `OUTP 3CF <plane>`,
then `MEMDUMPBIN A000 0000 1F40`. Restore read map 0 and Graphics
Controller selector 5 before each `RUN`. Keep all dumps and C probe
records under ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-after-row9-full \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-after-row9-full --target \
  poolrad_slot_a_after_row9_full_page_tests \
  poolrad_slot_a_full_ega_printer_page_probe
ctest --test-dir resources/build-slot-a-after-row9-full \
  -R '^poolrad_slot_a_after_row9_full_page_tests$' --output-on-failure
python3 tools/replay/slot_a_after_row9_full_page_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-ui-after-row9-full-p9-020 \
  --probe resources/build-slot-a-after-row9-full/poolrad_slot_a_full_ega_printer_page_probe
```

The audit verifies file and capture hashes, original and loaded
call/body spans, target relocation, frames, source hashes, EGA mode,
font hash, same-run page chain, and byte-exact sequential C outputs.
No game text, font bits, executable image, save, screenshot, RAM, or
page payload is tracked.
