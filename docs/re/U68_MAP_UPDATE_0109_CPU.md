# U68 +0109: raw-CPU map update (CPU/stack-effect form)

`pr_u68_map_update_0109_cpu` is a raw-CPU, full-state model of GAME.OVR unit
`0x68` `+0109..+0235`. The routine is already strict through the semantic
model [`overlay_u68_map_update.h`](U68_MAP_UPDATE_0109.md). However, the
strict queue lists `OVL_0068_0109` in `RAW_CPU_INTERFACE_CAVEATS` ("scalar
arguments and a segment view, not CPU/stack effects"). That caveat keeps its
raw-CPU callers from composing it. This model exposes the routine's full
CPU, stack and RAM effects so that U68 `+04DF` and `+0294` can compose it.
It adds no new body credit.

The VP approved it on 2026-10-18 (i17962) as the first step toward U68
`+04DF`.

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `0x68` `+0109..+0235`, GAME.OVR file offset `0x167D4` |
| Size | 301 bytes, 125 instructions, call-free, `RETF 6` at `+0233` |
| SHA-256 | `3f9a24eb07cda0c97f99a02fbd295fd7c4f55914e035d296b7e2b6ea7462d554` |

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

## Behaviour

The routine is entered after a far call (or `PUSH CS; CALL`):

| Frame slot | Contents |
| --- | --- |
| `[SP+4]` | the word s |
| `[SP+6]` | the word y |
| `[SP+8]` | the word x |

Only the low bytes of s, y and x are used; `RETF 6`.

1. **Bounds.** Signed bytes x or y outside 0..15 return (`JL`/`JG`/`JLE`).
2. **Store.** For s = 6, 4, 2 or 0, the map byte `ES:[DI+300h]` is updated.
   ES:DI is the far pointer `DS:[6A5Ch]` plus 16y + x (`CBW`, `SHL AX,4`,
   `ADD`), and the pointer is reloaded before the store:

   | s | New byte |
   | --- | --- |
   | 6 | `(b AND 3Fh) OR 40h` |
   | 4 | `(b AND CFh) OR 10h` |
   | 2 | `(b AND F3h) OR 04h` |
   | 0 | `(b AND FCh) OR 01h` |

   Any other s returns.
3. **Return.** `POP BP; RETF 6`.

**After a store:**

- AX = 16y, DX = x, CL = 4 (CH kept), BL = the new byte.
- ES:DI is the map pointer plus 16y + x.
- The arithmetic flags are `ADD DI,DX`'s; `SHL`'s undefined flags never
  survive.

**On an early return** the flags are the last `CMP`'s.

**Addressing.** Every operand is reread in source order. Offsets wrap at
16 bits, so `DI+300h` can wrap past `FFFF`. The code must sit at its original
offsets (IP = `0109h`).

## Validation

**Focused C test.** `test_overlay_u68_map_update_0109_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **32** hand-predicted checks:

- all four selectors, with every register and the flags;
- an invalid selector;
- x = 16 and y = −1;
- a map pointer whose `+300h` wraps;
- the guards.

**Unicorn differential.** `tools/overlay/u68_map_update_0109_cpu_audit.py`
runs the original unit bytes. After `RETF 6` it compares the whole 1 MiB RAM
and all 14 CPU words with **no mask**.

- **Cases: 247.**
- **Coverage.** 125/125 instruction starts and 16/16 conditional outcomes.
- **Inputs.** In-range and out-of-range signed coordinates, every selector
  class, wrapping map pointers, SS across A20, DF set, and a random
  background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **128
cases** (30 of them store) with no mask. Each compares all 14 CPU words, the
stack window and the whole data segment. The result SHA-256 is
`7907ddbb0086f78308ec2385a1ec812bb6096fbcc4563d8cbb7eecfea9d6b153`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u68_map_update_0109_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u68_map_update_0109_cpu_audit.py --unicorn --native --output resources/u68-0109/report.json
```

No classifier, queue or count ledger is changed. Removing `OVL_0068_0109`
from `RAW_CPU_INTERFACE_CAVEATS` is the verifying lane's call.
