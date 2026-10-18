# Pure callees of 0709:0768: SYSTEM set expansion 0D12 and resident 0899:0104

These are raw-CPU, full-state models of two pure callees of the port-bound
`0709:0768`, one of RES `00BA:16EC`'s direct callees. With `0709:0714`
(PR #1040), every pure callee of `0768` is now modelled. What remains of
`0768` is port-bound: `0709:0DD3`, `06A7`, `0BF5`. See
[the port-harness plan](RES_00BA_16EC_PORT_HARNESS.md#plan).

## SYSTEM 0AF8:0D12, set expansion

| Item | Value |
| --- | --- |
| Span | `0AF8:0D12..0D3B` of the expanded START.EXE |
| Size | 42 bytes, 18 instructions, `RETF 6` |
| SHA-256 | `e07f082267c72324f72e383e46d764968af96fceb4c7f7879de5871a5a721f6c` |

`pr_system_set_expand_0d12` uses the shared `pr_system_leaf_cpu` type. The
frame is `[SP+4]` the count word (BL data bytes, BH leading zero bytes),
`[SP+6]` the packed source and `[SP+0Ah]` the 32-byte destination. The
routine:

- runs `CLD`, then saves DS in DX and loads `ES:DI` and `DS:SI`;
- stores BH zero bytes, copies BL bytes, then stores `20h - BL - BH` zero
  bytes;
- restores DS and returns with `RETF 6`, leaving the destination pointer
  on the stack.

**The wrap.** The last count is a byte, so it wraps when BL + BH exceeds
32. For example, BL = `11h` and BH = `21h` give `EEh`, so 288 bytes are
written.

**Flags and registers.** The final flags are those of the second `SUB`.
On return AL is 0 (AH kept), BX is the count, CX is 0, DX is DS and DF is
clear.

**Addressing.** REP steps one byte at a time and rereads each source byte,
so overlapping strings behave as on the CPU.

## Resident 0899:0104

| Item | Value |
| --- | --- |
| Span | `0899:0104..0121` of the expanded START.EXE |
| Size | 30 bytes, 7 instructions reachable from the entry |
| SHA-256 | `1a9b687c96c69b4b791be00bd72730f6df05513ce522a18d53041f8846ce4d93` |

The routine runs `PUSH BP; MOV BP,SP; SUB SP,2; JMP 011E; MOV SP,BP; POP
BP; RETF`. Its flags are those of `SUB SP,2`. The `KeyPressed`/`ReadKey`
loop at `010C..011D` is never reached from `0104`.

## Validation

**Focused C test.** `test_res0768_pure_callees_cpu.c` runs under CTest with
ASan/UBSan. Both sources compile with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. It passes **13** hand-predicted checks:

- the expanded set bytes, registers and flags;
- the 288-byte wrap;
- `0104`'s frame and flags, including the borrow at SP 2;
- the guards.

**Unicorn differential.** `tools/overlay/res0768_pure_callees_cpu_audit.py
--routine 0d12|0104` compares the whole 1 MiB RAM and all 14 CPU words with
**no mask**.

- **`0D12`: 146 cases, 18/18 starts.** They include the wrap, overlapping
  source and destination, SI and DI wrapping past `FFFFh`, DF set, SS
  across A20, and a random background.
- **`0104`: 84 cases, 7/7 reachable starts.** They include SP 0 and 2,
  where the push wraps and `SUB` borrows.

**DOSBox-X oracle.** Each routine passes **32** normal-core/386 synthetic
cases with no mask. Each compares all 14 words, the stack window (which
holds `0D12`'s destination pointer) and the whole data segment.

| Routine | Result SHA-256 |
| --- | --- |
| `0D12` | `4c11fd06df53cc154fd728b61d4edc79ab6925b982355280190a54b065d2ddcf` |
| `0104` | `953e6478940c6bf85e0e1c4c47733f156aff0628b0468156d6db8d41672866c8` |

```sh
ctest --test-dir <build> -R poolrad_res0768_pure_callees_cpu --output-on-failure
for r in 0d12 0104; do
  PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1500s \
    python3 tools/overlay/res0768_pure_callees_cpu_audit.py --routine $r --unicorn --native --output resources/res0768-$r/report.json
done
```

No classifier, queue or count ledger is changed. Recording `0AF8:0D12` as
covered runtime, and crediting `RES_0899_0104`, are the verifying lane's
calls.
