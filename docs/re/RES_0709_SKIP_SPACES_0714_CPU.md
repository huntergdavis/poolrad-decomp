# Resident 0709:0714: raw-CPU nested space skip

`pr_r0709_skip_spaces_0714_cpu` is a raw-CPU, full-state model of the
START.EXE resident routine `0709:0714..074D`. It is a pure, call-free leaf
of the RES `00BA:16EC` closure: a nested helper of the port-bound
`0709:0768`, which it reaches through its parent's frame pointer. It moves
`0768` one callee closer to ready, following
[the port-harness plan](RES_00BA_16EC_PORT_HARNESS.md#plan).

## Exact original

| Item | Value |
| --- | --- |
| Span | `0709:0714..074D` of the expanded START.EXE |
| Size | 58 bytes, 19 instructions, `RETF 2` |
| SHA-256 | `67b1ce78e79b1e281051f3bc73e634213d904cedd5ec265a4bdf72f502af799c` |

## Behaviour

The routine is entered after a far call with the parent's frame pointer P
at `[SP+4]`. After `PUSH BP; MOV BP,SP`, every use rereads P from
`SS:[BP+6]`. The parent frame holds an index byte at `P-103h`, a limit byte
at `P-101h`, and text from `P-100h`. While the index is below the limit
(unsigned `JAE`) and the character at `SS:[P-100h+index]` is a space
(`CMP 20h; JNE`), `INC BYTE SS:[P-103h]; JMP 0717` repeats. It ends with
`POP BP; RETF 2`.

On return:

- the flags are those of the last `CMP`;
- AL is the index read last, and AH is 0 once a character was tested;
- DI is P, or P plus the index.

Every operand is reread in source order. The `INC` can land on the stack
word that holds P, so `max_loopbacks` caps the taken `JMP 0717`. It stops
at `+0717` with `LIMIT`. Without aliasing the loop takes at most the limit
minus the index.

## Validation

**Focused C test.** `test_resident_0709_skip_spaces_0714_cpu.c` runs under
CTest with ASan/UBSan, and the source compiles strictly. It passes **11**
hand-predicted checks:

- a two-space skip;
- a skip that reaches the limit;
- an index already at the limit;
- `LIMIT`;
- the guards.

**Unicorn differential.** `tools/overlay/res0709_skip_spaces_0714_cpu_audit.py`
runs **156** original-byte cases: 155 return and 1 `LIMIT`. It compares the
whole 1 MiB RAM and all 14 CPU words with **no mask**. Coverage is 19/19
instruction starts and 4/4 conditional outcomes, with up to 255
loop-backs. The cases include:

- an `INC` that rewrites P itself;
- SS across A20, DF set, and a random background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **32**
cases with no mask. The parent frame sits inside a 512-byte captured stack
window. Each case compares all 14 words, that window and the whole data
segment. The result SHA-256 is
`052cd242060a483455af4bc6d30675d22707dd6599c8923df7c06eea297af457`.

```sh
ctest --test-dir <build> -R poolrad_resident_0709_skip_spaces_0714_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1500s \
  python3 tools/overlay/res0709_skip_spaces_0714_cpu_audit.py --unicorn --native --output resources/res0709-skip/report.json
```

No classifier, queue or count ledger is changed. Crediting `RES_0709_0714`
is the verifying lane's call.
