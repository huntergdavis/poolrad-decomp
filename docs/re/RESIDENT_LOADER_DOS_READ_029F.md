# Saved resident loader DOS read at `12C6:029F`

The restored slot-A route reaches the two-byte `INT 21h` at loaded
`12C6:029F` with `AH=3Fh`. A native stop before and after the interrupt
establishes the service result and its observed memory effects. The bounded
C stage accepts an explicit host service event and stops at `+02A1`, before
the following `JB`; it does not emulate DOS or complete the loader.

Before debugging, `deja "Poolrad saved loader 12C6 029F INT 21h AH3F
read return CPU RAM DOS service"` found no indexed match. The merged
[read setup](RESIDENT_LOADER_READ_GATE_0291.md) and
[seek boundary](RESIDENT_LOADER_SEEK_028F.md) supplied the reached entry
and file cursor; both were checked again against the ignored inputs.

## Instruction and native result

| Input or span | Evidence |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Ignored `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Ignored slot-A `SAVGAMA.DAT` | 13,137 bytes; SHA-256 `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original `0AA2:029F..02A0` and loaded `12C6:029F..02A0` | Exact two-byte, one-instruction match; SHA-256 `855d16faaa027a1c2bf57ebdc2ca53925d942f65bf436cbaf69672c4c8227a8c` |
| Native numeric CPU transcript | 226 bytes; SHA-256 `0b890e9cd2f48622738739355618dfe9618c6998b23962aeb2cdb6ea2ab2659b` |
| Native 640 KiB RAM captures | Before SHA-256 `18a41c5e0e6f818ff57dcb79b759516abcce1dceac29c962037b15bb428fb665`; after SHA-256 `bdeee778d6df28218adc7bd684db9a683e6fe6318923a7422056a72f379b5280` |

The native stop enters at `CS:IP=12C6:029F`, `AX=3F87`, `BX=0005`,
`CX=0463`, `DS:DX=2CCD:0000`, `FLAGS=7246`. It leaves at
`CS:IP=12C6:02A1` with `AX=0463` and carry clear. All other captured
registers and flags are unchanged. The request is therefore a read of
1,123 bytes into physical address `0x2CCD0`. The native 640 KiB RAM
images differ at 1,122 addresses: 1,112 changed bytes in that buffer
and ten DOS-internal addresses in `0x208B8..0x208CE`. The 1,123
requested buffer writes include 11 same-value stores, which do not
appear in the RAM difference count.
The `14A0:[4426]` IOResult word remains zero at both stops.

The complete post-read buffer independently equals the hash-locked
`GAME.OVR` slice at the previously reached seek cursor `0x2AA87`, length
`0x463`, SHA-256
`c3badf3541d1395ac104a7441595b92f62628fd910081756f7b8973cc1185933`.
The count, buffer, and CPU result are thus supported by the ignored file
and native capture. The ten DOS-internal memory changes have no recovered
engine-level meaning; the audit carries them as an opaque native event.

## C boundary and limits

`pr_resident_loader_dos_read_029f` validates entry `CS:IP` and `AH`,
then requests an external service event. The event provides a post-service
CPU state and bounded, ordered guest-memory write journal. It is checked
before applying any write: every address must be in 1 MiB guest RAM, each
old byte must match the current or preceding journal value, and the
post-service address must be `12C6:02A1`. A missing service leaves CPU and
RAM unchanged and reports a blocked boundary. This is a model of the
measured external-service boundary, not a full C translation of a DOS
service or the enclosing loader. It earns no full-C or game-identity count.

The hash-locked local audit constructs the buffer writes from the ignored
`GAME.OVR` slice and appends the ten captured opaque DOS writes. Its
1,133-write journal reproduces every byte of the captured 640 KiB RAM
image and all 14 modeled CPU fields. Synthetic tests cover full and short
reads, carry/error, blocked service, malformed events, write bounds, and
invalid entry. The uncaptured upper 384 KiB of the model's 1 MiB RAM was
not compared with DOSBox. The following `JB` and later loader effects
remain outside this stage.

Reproduce locally with ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_dos_read_029f_audit.py
cmake -S . -B /tmp/poolrad-loader-dos-read-029f-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-dos-read-029f-build
ctest --test-dir /tmp/poolrad-loader-dos-read-029f-build --output-on-failure
```

Original files, RAM images, and numeric transcript remain under ignored
`resources/`; the audit prints only addresses, hashes, and counts.
