# Resident selector push at `0x0802:0x094A`

This note covers [issue #101](https://github.com/huntergdavis/poolrad-decomp/issues/101).
`pr_rec_res_0802_094a` is a **complete translation of the local resident
body and its AL/state behavior**. The original has one straight-line path
and no called dependency. Its game-authored versus linked-library ownership
is unresolved, so this change does not promote a verified game identity or
increase the strict *game* C count. The numeric state addresses stay raw.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Resident body and frame

The hash-locked unpacker maps `RES_0802:094A` to loaded image
`0x0896A..0x0897C`, **19 contiguous copied bytes** from packed file
`0x089A1..0x089B3`, with zero fill bytes. The loaded span SHA-256 is
`9cb0afed6f73df887b61f47778db5d55bd9063b59c8f6067ab0c38ec3350d205`.
Capstone 5.0.7 decodes all 19 bytes as eight instructions, with no branch
or call, ending at `0x0897A RETF 2`. The far Pascal frame has one pushed
WORD at `[BP+6]`; the procedure reads only its low byte.

| Loaded address | Instruction-level effect | C effect |
| --- | --- | --- |
| `0x0896A..0x0896B` | Establish BP frame. | Caller supplies a writable state object. |
| `0x0896D..0x08970` | Load byte `DS:0x5376`; store it to `DS:0x5377`. | `b5377 = b5376`. |
| `0x08973..0x08976` | Load byte `[BP+6]` into AL; store AL to `DS:0x5376`. | `b5376 = low(pushed_word)`. |
| `0x08979..0x0897C` | Restore BP and far-return, popping two argument bytes. | Return AL = `low(pushed_word)`. |

The original leaves AH unchanged; neither decoded overlay caller observes
AX after this call. The C contract exposes the proven AL byte. The preceding
value of `DS:0x5377` is overwritten. Repeated calls therefore save the
immediately preceding `0x5376`, including when the new low byte equals it.
There is no file I/O or lower runtime callback inside this 19-byte body.

The adjacent `RES_0802:095D` at loaded `0x0897D` copies `DS:0x5377` back
to `DS:0x5376`; it is a separate routine and is not translated here. One
of its two fixup-backed overlay calls is at unit `0x6F:+0x0664`, later in
the same public routine that calls this push helper. The resident file-open
helper reads `DS:0x5376` at loaded `0x0863A` and scales it by `0x1E` to
select a record at `DS:0x2614`. This verifies a selector role in the DAX
path; the table entries' semantic labels and this helper's authorship are
still uncertain.

## Both overlay callers and the exact pushed WORD

The [resident inventory](resident_loader_ledger.csv) records two
relocation-backed far calls to `0x0802:0x094A`. Both were independently
checked against the hash-locked `GAME.OVR` unit code:

| Caller | File call site | Argument evidence | After the call |
| --- | --- | --- | --- |
| Unit `0x25:+0x0C03`, inside class-1 ECL hook `+0x0A74` | `0x080D6` | At `+0x0A83`, AX holds `rel = address - 0x6B00`; on the `rel == 0x0312` path all preceding comparisons leave AX at `0x0312`. `+0x0BFF` replaces only AL with low(value), then `+0x0C02` pushes the **full WORD** `0x0300 \| (value & 0x00FF)`. | Jumps to the hook exit; AL is not used. |
| Unit `0x6F:+0x048C`, public entry 3 at `+0x0482` | `0x1E472` | `+0x0489 mov al,3; +0x048B push ax`. The low byte is exactly 3; AH is inherited and not established by this prologue. | Proceeds to record and runtime operations; AL is not used. |

The class-1 hook's 515-byte body at unit `0x25:+0x0A74` has SHA-256
`258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938`.
The 540-byte unit `0x6F:+0x0482` body has SHA-256
`a939d6fc0f5514a7efd9e9a08cbff48ef14ff41eddd13652284fd381489240aa`.
The new `pr_rec_res_0802_094a_hook(void *, uint16_t)` adapter fits the
merged class-1 hook's callback type without changing p2's source or API.
It intentionally discards the resident AL, as that caller does.

## Verification and prior work

Synthetic tests check save-before-replace ordering, repeated calls, AL,
all 256 possible high bytes against the same low byte, and an integrated
call through the merged class-1 hook. The integrated test supplies
`value = 0xFF42` and verifies that the hook reports/pushes `0x0342`, while
the resident state receives byte `0x42`. A second hook case verifies
the same pushed WORD for `value = 0x0042`. All data are synthetic.

```sh
cmake -S . -B /tmp/poolrad-issue-101-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-101-build
ctest --test-dir /tmp/poolrad-issue-101-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json /tmp/poolrad-issue-101-loader.json
```

Before implementation, `deja "poolrad RES_0802_094A 0x0896A ECL class 1
callback pushed WORD"` surfaced only the assignment. The broader
`deja "class 1 ECL store hook RES_0802_094A"` surfaced Claude's hook work.
Reused leads: the merged [hook investigation](ECL_STORE_HOOK_0A74.md),
the [resident loader map](RESIDENT_LOADER_MAP.md), the
[resident inventory](resident_loader_ledger.csv), and the
[file-open investigation](RESIDENT_DAX_FILE_OPEN.md). This work independently
rechecked both executable hashes, the resident copied span, the complete
eight-instruction body, fixup-backed caller addresses, the hook's full WORD,
and the unit `0x6F` argument shape. No original executable, archive, game
string, save, or derived media is committed.
