# Unit 0x5E +0x2879 party-derived value

`pr_u5e_party_value_2879` translates the complete call-free routine
at unit `0x5E:+0x2879..+0x2907`. It traverses the far linked list
headed at `DS:0x5D96/0x5D98`, conditionally accumulates two raw node
bytes, and, when its second sum is nonzero, writes the low result byte
to `DS:0x6E2F`. The byte fields and result's game meaning are not
named beyond their addresses.

Before decoding, `deja "Poolrad u5E 2879 party leaf full C"` and
`deja "u5e 2879"` found no indexed exact match. A broad `deja
"u6f"` query recovered earlier party-route research, but no u5E
implementation. This audit reuses the generated [overlay
classification](overlay_classification_summary.md) and
[fixup-aware callgraph](callgraph_summary.md) for candidate selection,
then independently verifies the original span, entry descriptor,
callsite fixups, and instruction paths against ignored DOS binaries.

| Original input/span | Evidence |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| u5E `+0x2879..+0x2907` | GAME file `0x15074..0x15102`; 143 bytes / 50 x86-16 instructions; SHA-256 `a474c72a13a1d9fde8a5b64f7d57ca2393afdd8a5e1861038dbf725454d3d4ec` |

The next prologue starts at `+0x2908`, so the 143-byte extent ends
exactly at `RETF` at `+0x2907`. Every direct branch targets an
instruction in this span; it makes **zero calls**. Unit 0x5E entry 19
maps stub `0068:007F` to `+0x2879`. The original fixup tables confirm
two far calls to that stub: unit `0x2F:+0x0895` within `+0x0880`,
and unit `0x33:+0x1F7F` within `+0x1E31`. This caller and party-head
evidence supports a game-specific role. It does not establish that
either call ran in the saved slot-A one-Up replay; native saved-route
reachability is unmeasured.

## Instruction-level behavior

`+0x2879..+0x2893` pushes BP, reserves eight bytes in SS, zeros
two 16-bit sums at `[BP-2]` and `[BP-4]`, reads the current party
head words, and stores them at `[BP-8]` and `[BP-6]`. Those SS words
and all far node accesses share the same A20-off 1 MiB RAM, so source
order matters when they alias another read or write.

At `+0x2896..+0x28E6`, each pass rereads the local far pointer. A
zero offset **and** segment ends the loop. If node byte `+0x10E`
equals 1, the routine adds node byte `+0x11B` to `[BP-4]` only when
node byte `+0x10D` is nonzero, and always adds node byte `+0x32` to
`[BP-2]`. Both sums wrap at 16 bits. It then reads the far next
pointer from node `+0x104/+0x106` and loops.

At `+0x28E8`, a zero `[BP-2]` leaves `DS:0x6E2F` unchanged. Otherwise
the source multiplies `[BP-4]` by 20, **discards the high word** with
`XOR DX,DX`, divides the low 16-bit product by `[BP-2]`, multiplies
that integer quotient by five through two shifts and one add, and
stores only AL at `DS:0x6E2F`. The public CPU result includes the
remainder in DX, the quotient in SI, arithmetic flags from the final
ADD, the last LES ES:DI, and the far return state. No divide by zero
is reached on a returning path.

## Parity and nontermination boundary

The hash-locked audit executes original bytes under Unicorn and the
C translation on **113 finite synthetic cases**: empty, filtered,
gated, mixed and 20-node lists; randomized fields; SS head/local,
node/sum, and
DS result/return-frame aliases; record and SS offset wrap; and a
read-only A20-crossing record. Every byte of 1 MiB RAM and every
returned 16-bit register, including FLAGS, agree. For the
A20-crossing read, the audit mirrors the synthetic lower RAM into
Unicorn's second MiB; the routine has no high-memory writes on that
case. The independent CTest covers the principal behavioral paths
and an output/return-frame alias under strict compiler warnings.

The DOS loop has no cycle check. With a disjoint record whose next
pointer is itself and whose eligibility byte is zero, the code never
changes that pointer, so the null exit cannot be reached. Original
bytes revisit `+0x2896` 83 times within a 1,000-instruction
observation; that instruction budget is an observation, not a
termination claim. The C API's positive caller-supplied `max_nodes`
returns `LIMIT` at the next `+0x289E` boundary after the specified
number of nodes. This is a host safety boundary for cyclic or longer
lists; it is not a DOS side effect. All finite parity cases use a
limit at least as large as their traversed list.

Reproduce locally, with original binaries retained only under ignored
`resources/`:

```sh
cmake -S . -B resources/build-u5e-2879 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u5e-2879
ctest --test-dir resources/build-u5e-2879 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u5e_party_value_2879_audit.py --unicorn
```

The audit prints only addresses, counts, and hashes. Original bytes,
RAM, saves, and game media remain ignored. Strict full-C and identity
accounting await PM parity review and merge.
