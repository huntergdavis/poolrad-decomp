# Slot A synthetic GOTO body

This adapter composes the recovered unit 1C `00F4` handler after the synthetic
GOTO dispatch in the slot A chain. It authenticates the handler, the two live
far-call stubs and the U25 loader and word helper against caller-supplied code
snapshots. It accepts only PC `993B` and the retained tag-01 operand `996B`;
the caller must supply immutable guest code, disjoint host objects and the
correct SYSTEM binding. It rejects data/stack and operand/stack aliasing before
starting. After starting, it exposes the raw handler's status and partial CPU
and memory effects, including the actual final return address.

The chain's prior predicate uses a synthetic zero window, and its earlier value
11 is synthetic. This is not evidence of same-invocation game provenance, native
`345B` parity, or a complete Up action. The retained operand shape is described
by the existing slot-A chain evidence; the raw handler and its limitations are
in `U1C_GOTO_00F4_CPU.md`. In particular, raw `OK` does not authenticate the
handler's final return address; consumers must inspect the returned CPU.
