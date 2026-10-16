# U19 row-14 selected-store launch and stop route

This source-only gate binds a prior cold-start launch route to the original
and retained loaded code leading to U19 `+02D8`. It supplies a bounded
[eight-stop template](U19_ROW14_SELECTED_STOP_TEMPLATE.json) for one future
same-run capture. It does not authenticate a new stop stream or establish a
first value-changing EGA writer, page selection, host RGB timing, or C parity.
Heavy native remained held.

Both bounded `deja` lookups for the selected U19 `+02D8` witness route and
host RGB/page-helper handoff timed out. The merged
[native credits-to-prompt route](NATIVE_COLD_START_CODEWORD_MENU_ROUTE.md),
[U19 key-read route](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md),
[strict target filter](U19_ROW14_TARGET_FILTER_PREFLIGHT.md), and
[paired event plan](U19_ROW14_SAME_RUN_EVENT_PLAN.md) supplied the previous
launch, caller, witness, and stop limits. This audit independently checks
the original-to-loaded U19 mapping and the template's bounded stop order.

## Pinned route and intervening gates

Ignored `START.EXE` and `GAME.OVR` retain SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The prior private no-argument run used the hash-pinned heavy debugger, waited
for credits RGB SHA-256
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`,
then sent one Return. It observed resident `0824:0122` calling the U19 stub,
the stub jumping to `38D3:00FD`, and a later U19 return at `0824:0127`.
Those are prior-run observations, not a continuous trace in the retained
row-14 capture.

The retained `+02DD` RAM has SHA-256
`84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699`.
Within it, resident `0824:0122` targets `0840:0025`; that patched stub
targets `38D3:00FD`; U19 `38D3:02D8` targets `0F2D:0626`.
The original resident call at image `+0122` targets `001C:0025` and hashes
`54e957158779fadaefe40e9504e2ea47cb99c7425ee051e8c2c849f2940596c7`;
its loaded five bytes hash
`8eae8facca4e09951d8568742355469f03771c781568754c37883c332fdb9e6e`.
The retained stub's five-byte SHA-256 is
`bf0c36dda99d8b3d11ab4b874d52757a90e1186a03e3fc7271d704c26634a052`.

U19 original `+00FD..+02DC` is 480 bytes, SHA-256
`f610b645b80150a4667efb87802cee3a76531bc7791dd02f75bd501ca93432d2`.
Its retained loaded counterpart hashes
`52f36b7575cbc5009ea1c9a597a0730b29cda4d7931073eedc5ab2fa4103595a`.
All 44 changed bytes are the segment WORDs of 22 exact far-call sites;
each adds the measured MZ load base `0x0824`, with zero other changed bytes.
The source `+00FD..+0110` entry gate is 20 bytes, SHA-256
`b998e4d2d6493b9a8c1352791d6ebc9293378c016e9a6298ccf15c9d4ae89d4c`.
Its `+0108` short jump rejoins at `+010D`. The 49-byte
`+023D..+026D` conditional-glyph gate hashes
`5abff77204a4fa392ef972f789d1e57a84495068a8e03aeb6d694c24c825ad3d`;
its `+0242` equal arm skips that optional glyph to `+026E`, where both
paths resume before `+02D8`. The bounded decode has no other branch or
return in `+00FD..+02DC`; every near/far callee must return normally to
reach the final print.

The last `+02C0..+02DC` setup is 29 original bytes, SHA-256
`47736a813d62c3d22ee798530485492669d834d20c4a2d448a8cb9bc33914a75`;
its loaded hash is
`ca56b7ab378d34e6a576559613f2784044bca4e02bd2795151f2763b0ab711b6`.
It pushes WORDs whose **low** bytes are column 1, row 14, and color 10;
the high bytes carry live register residue. Runtime `0AF8:0B0B` copies
the fixed source to a stack local at `[BP-0x50]`, then U19 far-calls
`0709:0626` at `+02D8` (loaded `0F2D:0626`). The printer's documented frame
uses far string `[BP+6]`, color `[BP+0xA]`, row `[BP+0xC]`, column
`[BP+0xE]`, and returns with `RETF 0xA`. No private pre-`+02D8`
CPU/full-RAM or printer-entry frame has been retained for this selected
row; the copied string and full argument WORDs cannot be certified from
the later `+02DD` RAM alone.

## Missing same-run checkpoint and bounded manifest

The public JSON template records prior PCs as **examples**. A future run
must derive the live MZ base, patched stub target, U19 CS, and resident
printer CS from its own earlier stops. It must verify the credits RGB and
single queued Return, then capture U19 entry, pre-`+02D8` CPU/full RAM,
printer-entry far caller frame, one selected drawer store whose effective
`ES:DI=A000:118F`, its exact next instruction, and the immediate U19
`+02DD` return in one ordered run token. The strict target filter caps
candidate store hits at 128; the merged event-plan checker requires
paired four-plane windows and restored Graphics Controller state.
No current retained artifact supplies the pre-`+02D8` or selected
instruction pair. Reusing the older `+02DD` RAM as if it were the new
same-run return would conflate runs.

Two follow-ons remain distinct: (1) a separately authorized native slot
to fill the eight-stop template and prove a selected CPU/plane event;
(2) the independent host RGB/page-helper return source map and later
same-run page/presentation correlation. There is no clock ETA before a
native slot and capture recipe are approved.

Reproduce the read-only source/template audit without launching DOS:

```sh
python3 tools/replay/cold_start_codeword_row14_selected_route_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --ram resources/oracle-runs/codeword-row14-p5-001/at-u19-02dd.ram \
  --template docs/re/U19_ROW14_SELECTED_STOP_TEMPLATE.json
```

Only offsets, hashes, counts, and a source-only template are public.
Original game bytes, selected text, RAM, EGA planes, RGB, and future stop
logs remain ignored under `resources/`. No native, first-writer, page/RGB,
strict C, or full-frame credit follows from this gate.
