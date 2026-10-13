# Saved resident loader DOS seek boundary `+028F`

The saved slot-A cold loader reaches loaded `12C6:028F INT 21h` with
`AX=4200h`. A fresh native run stopped immediately before the interrupt
and at its first returned instruction, `12C6:0291`. This bounded C stage
accepts a validated DOS-service event and advances exactly that one
instruction. It does not emulate DOS kernel internals, execute the later
file reads, or claim a complete enclosing loader/function translation.

Before debugging, `deja "Poolrad saved resident loader 12C6 028F INT
21h 4200 DOS seek native CPU RAM IOResult"` timed out during local index
refresh (exit 124). The merged [+0268 prefix note](RESIDENT_LOADER_0268.md)
provided the saved request and first-DOS-stop lead. This run independently
captured the before/after service state.

## Hash-locked instruction and native stops

| Input or span | Evidence |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Ignored slot-A `SAVGAMA.DAT` | 13,137 bytes; SHA-256 `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original `0AA2:028F..0290`, loaded `12C6:028F..0290` | Exact two-byte, one-instruction match; SHA-256 `855d16faaa027a1c2bf57ebdc2ca53925d942f65bf436cbaf69672c4c8227a8c` |
| Numeric CPU transcript | 226 bytes; SHA-256 `2828bfa0b49198e6c79f8fa8b5b50a5067a02eec7bb8d91b9ea78ce8f79c2f2c` |
| 655,360-byte RAM before / after | SHA-256 `bd0f46df996192228db2bed77c7e060823e6f459b4893d02c318500a9c5f8fc3` / `52fc48d9846dce42cb9481d803345d214c1eea6544d791fc36965992b36ff902` |

The direct native stops in ignored run `slot-a-loader-seek-028f-008` show:

| CPU field | Before DOS seek | First post-service instruction |
| --- | --- | --- |
| `CS:IP` | `12C6:028F` | `12C6:0291` |
| `AX,BX,CX,DX` | `4200,0005,0002,AA87` | `AA87,0005,0002,0002` |
| `DS,ES,SS:SP` | `14A0,08D0,1D16:3770` | unchanged |
| `SI,DI,BP,FLAGS` | `44BA,0824,3794,7202` | unchanged |

`FS=GS=0000` at both stops. The returned `DX:AX=0002:AA87` equals
the requested absolute offset, with carry clear. Exactly 26 captured
guest-RAM bytes changed, in physical ranges `0x0816E..0x0816F` and
`0x208B8..0x208CF`. The ordered write journal has SHA-256
`ac6d267601b857130cdc6fa3335eba9015427080c82ff20b43a17f0a94232b9a`.
The guest `DS:[4426]` IOResult word remained `0000`; no IOResult call
occurs in this one-instruction stage. The next instruction at `+0291`
loads DS for a later DOS read, which remains outside this model. The
underlying DOS handle's filename has not been proven here.

## C service boundary and parity limit

`pr_resident_loader_seek_028f` requires the reached `CS:IP` and
`AX=4200h` request. With no service callback, it returns
`BLOCKED_DOS_SERVICE` without changing CPU or RAM. A callback returns
the post-interrupt CPU and an ordered guest-RAM write journal; the stage
checks target `CS:IP`, stack preservation, journal capacity, physical
bounds, and each prior byte before applying anything. Malformed events
leave state untouched. No saved result tuple or file owner is embedded
in the C implementation.

The private audit derives returned `DX:AX` by seeking a generic host
open file to the **input** `CX:DX` offset. It supplies the separately
captured DOS guest-RAM writes as the service event. The resulting C state
matches all 14 modeled CPU fields and every byte of the captured 640 KiB
RAM image. This is event-application parity, conditional on the
supplied DOS write journal; it does not independently synthesize the
DOSBox kernel's 26 writes. The upper 384 KiB of the C RAM space was not
captured. Synthetic tests also exercise alternate absolute positions,
a carry-set DOS error event, missing/blocked service, repeated-address
writes, and malformed journals. Only the saved success event has a
direct native before/after pair.

Reproduce locally with ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_seek_028f_audit.py
cmake -S . -B /tmp/poolrad-loader-seek-028f-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-seek-028f-build
ctest --test-dir /tmp/poolrad-loader-seek-028f-build --output-on-failure
```

The original executable, save, screen, CPU transcript, and RAM captures
remain under ignored `resources/`. The audit prints only addresses,
hashes, and counts.
