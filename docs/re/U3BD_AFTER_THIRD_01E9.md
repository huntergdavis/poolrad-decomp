# U3BD entry-5 parent setup after the third resident call

## Boundary and provenance

This is the 33-byte overlay span `2D14:+01E9..+020A`, from the
**provisional original-byte** third `RES_0464:+2632` return to the next
far-call instruction. The C model stops before that fourth call and does
not claim a complete parent routine or any strict/full-C count.

Before implementation, `deja "Poolrad U3BD entry5 01E9 020A after third
RES_0464 2632 parent stack argument setup"` found no indexed match.
Reused the local third-call [address note](RESIDENT_464_2632_THIRD_CALL.md)
and its hash-locked original-byte return. Direct DOS debugger `+01E9`
return and `+020A` pre-call RAM/CPU captures have been requested from pD;
neither is claimed here.

The original `GAME.OVR` file span `0x2EAE0..0x2EB01` and loaded overlay
span have the same 33-byte SHA-256,
`dab914875d9752e2374a769ad20b7ba91674c38653451e9f346df3f8c2eb6092`.
There are 15 executed instructions before the `+020A` boundary, including
nine argument-word pushes and no conditional branch. The source reads
`DS:6A63` as the low byte of one pushed word and far-pointer words at
`DS:4A22` and `DS:4A20`. The saved route has `DS:6A63=6` and the far
pointer `3DE4:0008`. FLAGS stay unchanged.

The provisional `+01E9` CPU tuple, ordered `CS IP AX BX CX DX SI DI BP SP
DS ES SS FLAGS`, is

`2D14 01E9 0001 3E4E 0000 14A0 3E68 007A 3E98 3E98 14A0 3DE4 1D16 7246`.

After nine pushes, the next call prestate is

`2D14 020A 0006 3E4E 0000 14A0 3E68 260A 3E98 3E86 14A0 3DE4 1D16 7246`.

Original-byte replay changes only three stack bytes between those
boundaries. Its full 640 KiB RAM SHA-256 at `+020A` is
`454dec9cca598103bbb945e97557649c655801c10a5ae58267b9388a4d2b99da`.
The C model matches that provisional original-byte RAM and all 14 CPU
fields exactly. Synthetic tests check push order, a different source byte,
invalid host input, and an unsupported CPU state with no mutation.

## Remaining direct gate

The comparison above is against original-byte replay from an earlier
provisional return. The public replay accepts an optional direct native
`+020A` image when pD supplies one. This parent segment and fourth call
must remain separate from complete-routine accounting until their reached
native boundaries are independently checked.
