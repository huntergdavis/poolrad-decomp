# Resident 0709:0000: raw-CPU EGA register reset (first port-bound body)

`pr_r0709_ega_reset_0000_cpu` is a raw-CPU, full-state model of the
START.EXE resident routine `0709:0000..0070`. It is the first port-bound
routine of the RES `00BA:16EC` closure, and step 1 of
[the port-harness plan](RES_00BA_16EC_PORT_HARNESS.md#plan).

It introduces the shared port environment `pr_port_env`
(`poolrad/recovered/port_env.h`) and proves the guest-side tracer on the
stock DOSBox-X. No emulator code is involved; see the spec's licence
boundary.

## Exact original

| Item | Value |
| --- | --- |
| Span | `0709:0000..0070` of the expanded START.EXE |
| Size | 113 bytes, 58 instructions, call-free, `RETF` at `0070` |
| SHA-256 | `5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5` |

## Behaviour

The routine runs `PUSH BP; MOV BP,SP`, then eighteen byte OUTs, each
`MOV AL,imm8; MOV DX,imm16; OUT DX,AL`. It ends with `POP BP; RETF`. The
OUTs reset:

- the graphics controller, index and data at `3CEh`/`3CFh`: registers 0–5
  := 0, 7 := `0Fh` and 8 := `FFh`;
- the sequencer Map Mask: `3C4h` index 2, then `3C5h` := `0Fh`.

No instruction changes FLAGS. After a return AL is `0Fh` (AH is kept) and
DX is `03C5h`.

**The port environment.** Each OUT goes, in order, to `env->out(port,
value, 1, CS, IP)` at its own IP (`0008h + 6k`). A nonzero return stops the
model with `PORT` at that OUT, with AL and DX loaded. `ARGUMENT` covers
null RAM, CPU, environment or callback, RAM other than 1 MiB, or IP not
`0000h`.

## Validation

**Focused C test.** `test_resident_0709_ega_reset_0000_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **27** checks:

- every OUT's port, value, CS and IP;
- the final state;
- a refusal at the fourth OUT;
- the guards.

**Unicorn differential.** `tools/overlay/res0709_ega_reset_0000_cpu_audit.py`
runs the original bytes with `UC_HOOK_INSN` on `OUT`. The whole 1 MiB RAM,
all 14 CPU words and the **ordered OUT events** (port, value, size, CS:IP)
must equal the C model's, with **no mask**. There are **66** cases: 63
return and 3 `PORT`, with refusals at OUTs 0, 5 and 17. They cover 1,156
OUT events and 58/58 instruction starts.

**DOSBox-X oracle, first port-harness prototype.** Each of **32** cases
runs twice in the synthetic COM:

- **Plain run.** It compares all 14 CPU words, the stack window and the
  whole data segment with the C model, with no mask.
- **Traced run.** It sets TF before the far call. A guest `INT 1` handler
  logs every `OUT` the routine reaches (`EE`/`EF`/`E6`/`E7`) as a 32-byte
  event in the spec's format. All **576** events equal the C model's port
  environment.
- The result SHA-256 is
  `0aad7592bd3bc100b897fec318a10fe4cd52f0f77ed41833479bd6e9d8ada393`.

```sh
ctest --test-dir <build> -R poolrad_resident_0709_ega_reset_0000_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1500s \
  python3 tools/overlay/res0709_ega_reset_0000_cpu_audit.py --unicorn --native --output resources/res0709-ega/report.json
```

Original bytes, COM images and traces stay under ignored `resources/`. No
classifier, queue or count ledger is changed. Crediting `RES_0709_0000` is
the verifying lane's call.
