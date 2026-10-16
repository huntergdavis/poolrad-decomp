# U9E entry8 bounded full-state C composition

The source screen in
[U9E_ENTRY8_FULLSTATE_COMPOSITION_SCREEN.md](U9E_ENTRY8_FULLSTATE_COMPOSITION_SCREEN.md)
was accepted by PM (i7915) before this C edit. This wrapper joins only the
merged first-pass prefix, +0E0E SYSTEM call adapter, +0E13 postcall,
+0E05 loopback on later iterations, and +0E1F RETF4 tail. It starts
at loaded U9E CS:+0DE3 with the original far-call frame. B0:0048
stub dispatch and U7F caller behavior are outside this API.

The wrapper copies all 14 CPU words by name across distinct component
types and passes the same 1 MiB A20-off RAM pointer throughout. It
does not detach the seed, range, stack locals, or return words. Prefix
count zero proceeds directly to the tail. Nonzero count enters
SYSTEM; only a normal return flows to postcall. An unequal index/count
takes +0E05 and rereads the live range before the next call. The caller
supplies a maximum number of SYSTEM call slices; exceeding it stops
at +0E0E with the prior live effects retained. A zero cap still allows
the count-zero tail. Limit is a host boundary, not an original branch.

The API retains explicit argument and per-component rejection,
SYSTEM argument/failure/near-transfer, changed far-return transfer,
and limit statuses. A later component rejection leaves preceding
effects live. The tail reads the actual wrapped SS return words; it
does not force a predicted caller CS:IP. The source assumes the loaded
U9E and SYSTEM instruction bodies match the pinned original bytes,
as do the separate component APIs. Their original/loaded audits all
passed in the pre-edit screen.

Focused synthetic C cases compare the wrapper's entire RAM and CPU14
with separately invoked merged components for count 0, one zero-range
roll, two range-one rolls, wrapped SS:FFFF return IP, a DS seed alias
that changes the second roll's range, a one-call host limit, and an AF-carry loopback at index 0x0F.
Independent numeric assertions pin the seed and AL for those cases.
Additional cases check unchanged bad arguments/code-write rejection,
staged SYSTEM code-alias rejection after prefix effects, and a seed
write that redirects the far return or SYSTEM near return. These are controlled C
composition checks, not original-instruction or natural native parity.
No complete game-function or strict-C count credit follows.
