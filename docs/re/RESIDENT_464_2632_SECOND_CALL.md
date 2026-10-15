# Resident 0464:+2632 second saved-view caller gate

## Scope and reuse

This models the saved run014 overlay caller `2D14:01BE` through its far call
to loaded `0C88:2632` and return to `2D14:01C3`. The resident body is the
previously reviewed **one-row** bounded composition from PR #484; this work
adds the caller instruction and an independent original-byte replay. It does
not translate another resident path or claim complete-function, strict-C, or
game-identity credit. The third caller at `+01E4` is outside this gate.

Before this work, `deja "Poolrad RES_0464 2632 second run014 call 01BE 01C3
original byte CPU full RAM parity"` was run with a 35-second bound and timed
out. Reused evidence: merged
[one-row composition](RESIDENT_464_2632_RUN014_COMPOSED.md) from PR #484 and
pD's numeric same-run direct stops in its local
`U3BD_ENTRY5_05FD_NATIVE_FRAME` note. A fresh independent direct debugger
tuple was requested from pD; the recorded tuples are also checked by the
original-byte replay. Media and RAM images remain ignored.

## Hash-locked call and frame

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its `0464:+2632..+2A0D` body has 987 bytes at file `0x06CAB..0x07086`,
SHA-256
`ee35f98f513d5144e92e55e2afafbe176bba9ab65acc905ea151c19c5a9d713a`.
The original five-byte far call at `GAME.OVR` file `0x2EAB5` targets
`0464:2632`, SHA-256
`a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe`.
The loaded call at `2D14:01BE` targets `0C88:2632`, SHA-256
`0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4`.

The same-run caller CPU tuple, in order `CS IP AX BX CX DX SI DI BP SP DS
ES SS FLAGS`, is

`2D14 01BE 000F 3E4E 0000 14A0 3E68 260A 3E98 3E86 14A0 3DE4 1D16 7246`.

The far call preserves FLAGS, pushes caller CS then return IP, and enters
`0C88:2632` with `SP=3E82`. The resulting entry CPU is

`0C88 2632 000F 3E4E 0000 14A0 3E68 260A 3E98 3E82 14A0 3DE4 1D16 7246`.

The native caller-pre RAM SHA-256 is
`4fab836fb3c536e5ea78e6be81765c3c01aa8d86566a919c810eb749763beac0`;
the resident-entry image is
`7a4593ce6f8043fda34209915e84f1f936bdb6dccbc9e72d2fb24a20a5c2cd2a`.
Exactly one stack byte differs between those two images. The C wrapper
restores both overwritten stack words and the CPU if the resident model's
preflight rejects the path.

## Return and comparisons

The reviewed one-row body visits 14 native instruction boundaries and
changes 44 object bytes, six dirty/extent metadata bytes, and seven
stack/frame bytes from resident entry to return. Including the far call,
the same-run `+01BE` caller-pre to `+01C3` return changes 58 RAM bytes:
44 object, six metadata, eight stack/frame. The return RAM SHA-256 is
`722cac9dd2a9c8e002649255183878a5687054ed65f19105f09d228ea0cf154c`.
The return CPU is

`2D14 01C3 0000 3E4E 0000 14A0 3E68 0076 3E98 3E98 14A0 3DE4 1D16 7246`.

The optional C private comparison starts at the unchanged `+01BE` native
image, checks full 640 KiB RAM and all 14 CPU fields at resident `+2632`,
then checks full RAM and CPU at caller `+01C3`: zero differences. Synthetic
tests check the far-call frame, object and metadata effects, invalid host,
and unsupported path with no mutation.

The optional `tools/replay/resident_464_second_call_unicorn.py` verifies
the original executable, resident span, original/loaded call sites, and
same-run capture hashes, then executes the loaded original instructions.
It reaches resident entry at instruction 2 and caller return at instruction
612. Both full RAM images and all 14 CPU fields match the recorded native
stops and the C model. This is a same-run original-byte comparison; it does
not establish parity for other `+2632` arguments or branches.
