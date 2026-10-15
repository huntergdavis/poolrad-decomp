# Resident 0464:+2632 third saved-view caller gate

## Scope and dependency

The saved run014 route calls loaded `0C88:2632` a third time from overlay
`2D14:01E4`, returning to `2D14:01E9` in original-byte replay. This branch
stacks the PM-reviewed first-call multirow composer from PR #496 while its
GitHub merge is queued. It adds an exact two-row, mask-`0x0007` entry and a
separate caller wrapper. It does **not** establish the complete resident
routine, strict full-C credit, or the complete parent path.

Before tracing, `deja "Poolrad run014 third RES_0464 2632 call U3BD 01E4
01E9 return native parity"` timed out after a 30-second bound. Reused
the merged one-row composition from PR #484, the PM-reviewed first-call
multirow composition from PR #496, and pD's same-run saved RAM images.

## Address and state evidence

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Resident `+2632..+2A0D` is 987 bytes at file `0x06CAB..0x07086`, SHA-256
`ee35f98f513d5144e92e55e2afafbe176bba9ab65acc905ea151c19c5a9d713a`.
The original five-byte far call at `GAME.OVR` file `0x2EADB` targets
`0464:2632`, SHA-256
`a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe`.
The loaded call at `2D14:01E4` targets `0C88:2632`, SHA-256
`0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4`.

Starting from the directly measured second-call return `+01C3`, original
loaded instructions reach the saved third caller pre-image at `+01E4`
after 16 instructions and the saved resident entry at `+2632` after 17.
Full 640 KiB RAM matches pD's same-run saved images at both points with
zero differing bytes. Their SHA-256 hashes are, respectively,
`99fd4ea498e847b2ed225d3c399b1ebe0cb8d2db203664dd7ad3605e31c9075b`
and
`725ba34a5bba4f591c03125530ca7f652b215ad41527b8b235563241240c3ab9`.
The five-byte far call changes one stack byte between them. The caller
passes selection mask `0x0007`, two rows starting at 60, 11 four-byte
elements per row, horizontal index 2, and object `3DE4:0008` with width
21 and height 168. Video state `DS:4A1E` is 1.

Original-byte control flow takes the positive-X, nonnegative-row route
through `+28E0`, makes 22 runtime `Move` calls at `+29C6`, takes the
outer-loop backedge at `+2A04` once, then returns through `+2A0A`.
The original-byte return at `+01E9` is reached at instruction 1017 from
the measured second return. Its RAM SHA-256, subsequently confirmed by
the direct native return, is
`d5876edf3a5bab07f2bcf4531a6a0d804837a380dab89a6e3716e01da654f24b`;
the original-byte CPU tuple, ordered `CS IP AX BX CX DX SI DI BP SP DS ES
SS FLAGS`, is

`2D14 01E9 0001 3E4E 0000 14A0 3E68 007A 3E98 3E98 14A0 3DE4 1D16 7246`.

The caller-pre to return change set has 88 bytes: 66 object,
12 dirty/extent metadata, and ten stack/frame, with none outside those
regions. The two-row C composition matches that original-byte and direct return
across full 640 KiB RAM and all 14 CPU fields, and matches the saved native
entry RAM after the far-call push. The entry CPU comparison is against
original-byte replay. Synthetic tests exercise both
rows, the three selected mask bytes, metadata, return frame, and unsupported
no-mutation paths.

## Direct return gate

pD subsequently captured a direct same-run DOS debugger return at
`2D14:+01E9`. Its private 640 KiB RAM SHA-256 is
`d5876edf3a5bab07f2bcf4531a6a0d804837a380dab89a6e3716e01da654f24b`.
The original-byte replay and C third-call return each differ from that
direct image by zero bytes; the C entry observer reports one check and
zero errors. All 14 C CPU fields match the direct CPU tuple above. This
passes the bounded third-call return gate, but does not establish the
complete resident routine or a strict/full-C count. The parent caller
instructions after `+01E9` remain a separate follow-on.
