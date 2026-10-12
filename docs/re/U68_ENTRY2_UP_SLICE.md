# Unit 0x68 entry 2 (+0x0989): bounded extended-Up path

`pr_u68_entry2_up_slice` models one route through DOS v1.3 `GAME.OVR` unit
`0x68` public entry 2: selector `[0x49FA] == 1`, then the input callee
reports an extended key with `AL == 0x48`. It joins the merged Up preflight
at `+0x0AB2` and stops after this invocation's `RETF`. It is an **Up-path
slice**, not a translation of every entry-2 key and selector path. The
host must supply the private prompt preparation, both input outputs, the
resident display call, and the optional exit call. No saved one-Up C parity,
full-C credit, or identity-count change is claimed.

Before decoding, `deja "Poolrad unit 0x68 entry2 +0x0989 saved Up 0x48 prompt
resident 290B extended key"` found the earlier Up-preflight trace. Reused
and byte-checked: [the +0x068D proof](U68_UP_PREFLIGHT_068D.md),
[the gameplay-loop caller](GAMEPLAY_PATH.md),
[the resident 290B boundary](RESIDENT_ROUTE_290B.md), and
[the saved script gate](SLOT_A_UP_ECL_GATE.md). Those notes establish the
caller context and the separate merged callee; the entry-2 span and branches
below were decoded independently from the ignored binary.

## Provenance and extent

| Ignored input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

Unit `0x68` starts at file `0x166CB` and has run-time segment `0x72`.
Entry 2 begins at `+0x0989`, file `0x17054`. The code ends with `RETF` at
`+0x0D4B`, file `0x17416`: **963 bytes, 374 contiguous x86-16
instructions**, SHA-256
`6b79d88f95710c9ceb1f9d6d05a669d9125efd1c0ca34ec9f0911c35dc9e00f8`.
The ledger's 996-byte bound includes 33 bytes of private string data at
`+0x0D4C..+0x0D6C` (SHA-256
`7806b45fdd770565850b2f7bc5bf5387a8b65cec00f8d8298d208e93b79986c6`);
the next prologue is `+0x0D6D`. No literal or original byte is in this
repository. The entry reserves `0x3C` local bytes, has no caller argument,
and returns its saved key byte in `AL` by plain `RETF`.

The gameplay-loop call at unit `0x1C:+0x3395` reaches this public entry.
The path here has two near/far dependencies: `push cs; call +0x068D` at
`+0x0AB2` and `lcall 0xBA:290B` at `+0x0B9B`. The latter is a bounded
resident display model elsewhere in the tree, but this API leaves its
full side effects to a reached host hook. Two other reached far calls are
runtime `0xAF8:0x0B0B` at `+0x09B2` and input `0x3C5:0x002A` at
`+0x09D0`; the exit arm calls `0x3F1:0x0137` at `+0x0D35` only when
`[0x84E4] == 0`.

## Byte-checked path and effects

| Unit offsets | Instruction result and C boundary |
| --- | --- |
| `+0x098F..+0x0995` | `LES` window 1 through `[0x49D6]`; clear its word `+0x592` before reading the selector. The model bounds this two-byte write. |
| `+0x099A..+0x09A1` | `[0x49FA] != 1` jumps to the separate selector-2..4 ladder at `+0x0BAC`. The slice stops there, preserving the clear. |
| `+0x09A4..+0x09D0` | Local movement flag starts at zero. `0xAF8:0x0B0B` copies private `CS:0x097A` into the local prompt, then `0x3C5:0x002A` consumes `DS:0x04B6` and five byte-valued controls (low bytes `0x0D,0x0A,0x0F,1,0`). It returns key `AL`; its by-reference output is local `[BP-4]`, the extended flag. Both are required host outputs. The high bytes of `AX` at the byte pushes are not asserted. |
| `+0x09D5..+0x09DE`, `+0x0AAA..+0x0AB2` | Save key at `[BP-3]`; flag zero enters the separate character-key ladder at `+0x09E1`. Flag nonzero selects the scan-code ladder; only `AL == 0x48` takes `push cs; call +0x068D`. Other scan codes leave this slice at the `+0x0ABC` ladder. |
| `+0x0AB5..+0x0BA9` | Set local movement flag to 1; jump to `lcall 0xBA:290B` at `+0x0B9B`; its return reaches the nonzero-flag exit at `+0x0BA9`, never the prompt loop at `+0x09A8`. |
| `+0x0D1C..+0x0D3F` | Copy the **current** `[0x49FA]` byte into `[0x49F9]` after the resident call. If `[0x84E4] == 0`, call `0x3F1:0x0137` with four byte-valued pushed words (low bytes `1,0x11,0x26,0x16`), then write `[0x84E4] = 1`. |
| `+0x0D3F..+0x0D4B` | Reload saved key byte into `AL`, restore frame, `RETF`. |

`pr_u68_up_preflight` is called directly and owns its window-1 word
`+0x5AA`, party-coordinate clamp and field lookup contract. This slice
does not copy that implementation. The current selector is re-read after
the resident hook, which can change host state. `stop_offset` reports
the first unsupported branch or missing/failed reached hook; earlier
effects remain visible. No unneeded future hook is required.

The saved slot-A frame has selector 1, party `(15,4)` facing 6, and
window-1 gate word `+0x5AA == 0` (the linked notes provide the save
hash). These facts make this path relevant to the one-Up observation,
but the live input callee's full output and resident effects are not yet
captured as a C-native route. The DOS native write watch proves a later
window-0 minute change through a different call chain; this model does
not reproduce that change or assert a native guest-PC trace.

## Synthetic verification and limits

`test_overlay_u68_entry2_up.c` checks the successful call order
`prompt preparation → input → preflight → resident → optional exit`,
word `+0x592` clearing, `+0x5AA` preflight clearing, current-selector
copy, conditional exit, and returned `AL`. It checks the selector,
character-key, and non-Up scan-code boundaries; already-set exit flag;
missing/failed reached hooks (including an empty input callback that
does not provide its outputs); preflight failure after earlier prompt
effects; and the initial window bound. All memory, map, direction, and
input data are synthetic. The full entry's other keys, selector-2..4
prompt, string contents, resident display output, and exit callee
effects remain outside this API.
