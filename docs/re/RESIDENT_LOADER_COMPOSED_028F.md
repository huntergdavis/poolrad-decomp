# Saved resident loader seek/read/return composition

The saved slot-A cold-loader route reaches loaded `12C6:028F` before a
DOS seek and returns to its caller at `12C6:03AE`. This composition calls
the merged [+028F seek](RESIDENT_LOADER_SEEK_028F.md),
[+0291 read setup](RESIDENT_LOADER_READ_GATE_0291.md),
[+029F DOS read](RESIDENT_LOADER_DOS_READ_029F.md), and
[+02A1 branch](RESIDENT_LOADER_READ_BRANCH_02A1.md) stages in order,
then executes the two-instruction `+02E1` far-return epilogue. Seek and
read results remain explicit host events. This is a runnable saved-route
model, not a full C translation of the enclosing loader or DOS, and it
adds no full-C or game-identity credit.

Before debugging, `deja "Poolrad saved resident loader 12C6 028F seek
0291 read 029F INT21 02A1 branch 02E1 POP DS RETF 2 caller
composition"` and a shorter seek/read query found no indexed match.
The four merged component notes above supplied the starting boundaries;
this work independently captured one continuous native run through the
caller.

## Same-run DOS evidence

| Input or capture | Evidence |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Ignored `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Ignored slot-A `SAVGAMA.DAT` | 13,137 bytes; SHA-256 `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original `0AA2:02E1..02E4` and loaded `12C6:02E1..02E4` | Exact four-byte, two-instruction `POP DS; RETF 2` match; SHA-256 `215e5153fb4987a2421c32058133392d054b5a365b2c7b6ba7eb7cc51618a992` |
| Six-stop numeric CPU transcript | 487 bytes; SHA-256 `d1d232a9e94bb1b14ceadc27ca26157956b548228d94ee371b995eab55009365` |

The hash-pinned private heavy debugger stopped in one saved-slot run at
`12C6:028F`, `+0291`, `+029F`, `+02A1`, `+02E1`, and the caller
`12C6:03AE`. All six RAM images cover physical `0x00000..0x9FFFF`.

| Stop | `AX,CX,DX` | `DS` | `SP` | `FLAGS` | RAM SHA-256 |
| --- | --- | --- | --- | --- | --- |
| `+028F` | `4200,0002,AA87` | `14A0` | `3770` | `7202` | `1583efcf9647a21a0cffde8886abd4c4799e3958f65da2bd043d9a8636719282` |
| `+0291` | `AA87,0002,0002` | `14A0` | `3770` | `7202` | `d804f2081e95b2086bceab73cd0ba520fc754aba8027271da3ac782d1e777be9` |
| `+029F` | `3F87,0463,0000` | `2CCD` | `3770` | `7246` | same as `+0291` |
| `+02A1` | `0463,0463,0000` | `2CCD` | `3770` | `7246` | `991396a012b20c4494063e7ef940832b7dabce071ef95e681ee0804eb993003d` |
| `+02E1` | `0000,0000,0000` | `2D14` | `3770` | `7246` | same as `+02A1` |
| `+03AE` | `0000,0000,0000` | `14A0` | `3778` | `7246` | same as `+02A1` |

Other captured CPU fields are unchanged apart from the listed `CS:IP`;
`BX=0005`, `ES=08D0`, and `SS=1D16`. The seek returns the requested
absolute offset `DX:AX=0002:AA87`. The first read requests `CX=0463`
bytes into `DS:DX=2CCD:0000`, and all 1,123 destination bytes equal
the ignored `GAME.OVR` slice at cursor `0x2AA87`, SHA-256
`c3badf3541d1395ac104a7441595b92f62628fd910081756f7b8973cc1185933`.
The seek changes 26 RAM bytes; the read changes 1,122, including 1,112
changed buffer bytes and ten changed DOS-internal bytes. The read's
1,123 buffer writes include 11 same-value stores. Setup, branch, and
far return change no captured RAM byte.

At `+02E1`, the guest stack at `SS:SP=1D16:3770` holds restored DS
`14A0`, return IP `03AE`, return CS `12C6`, and the one-word argument
removed by `RETF 2`. The observed caller has `DS=14A0`,
`CS:IP=12C6:03AE`, and `SP=3778`. The next original caller
instruction is `POP ES`; this model stops before it. The saved
`ES:[000A]=0` skips the optional second DOS read at `+02C0`.

## Composed C boundary

`pr_resident_loader_composed_028f` accepts separate seek/read callbacks
and bounded write journals. It can resume at any of the four merged
component boundaries or at `+02E1`. A missing callback returns at the
pending `INT 21h`; earlier completed stages remain applied. The
optional second read returns as a separate `+02C0` boundary. At
`+02E1`, the C code reads the saved DS and far return from guest stack
RAM and advances SP by eight bytes, with 16-bit stack and 20-bit
physical wrapping. It embeds no saved DOS result, game-file contents,
or caller address.

The local audit supplies seek/read service events from the ignored
same-run captures and the independently pinned `GAME.OVR` slice. Its
26-write seek journal describes observed net changes; it does not
recover invisible same-value DOS writes. The read journal includes all
1,123 buffer writes plus ten captured DOS-internal net changes. Their
combined event-journal SHA-256 is
`5e3fa719599c9144159453a89aad59d491875e68e794aa55215af1ada90838ab`.
With those explicit events, the C path matches all 14 modeled CPU
fields and every byte of the captured 640 KiB caller RAM image.
Synthetic tests cover blocked and resumed services, success, malformed
events, short/error reads, the optional second-read boundary, stack
wrap, and invalid entry. Only the saved route has direct native
end-to-end evidence; the upper 384 KiB of guest RAM was not captured.

Reproduce locally with ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_composed_028f_audit.py
cmake -S . -B /tmp/poolrad-loader-composed-028f-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-composed-028f-build
ctest --test-dir /tmp/poolrad-loader-composed-028f-build --output-on-failure
```

To recapture, prepare a fresh private run with
`tools/oracle/prepare_run.sh`, launch the hash-pinned heavy DOSBox-X
through `tools/oracle/launch.sh`, stop at the six loaded addresses
above, and use `EV` plus `MEMDUMPBIN 0000 0000 A0000` at each stop.
The original media, screen, transcript, and RAM captures remain under
ignored `resources/`; the public audit prints only addresses, hashes,
and counts.
