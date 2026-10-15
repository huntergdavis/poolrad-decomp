# U3B0 +087B: bounded carry branch

Original DOS v1.3 GAME.OVR U3B0 +087B bytes are `73 3D`,
JAE rel8 +0x3D. The ignored 206105-byte DOS image has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3B0 file base is `0x2C8CF`; owned span `[0x2D14A,0x2D14C)`
has SHA-256 `d34d6184e530e0c84feda884df2625860aaf8c0fe217b44dc038c2df09932c91`.

This supplied-state gate verifies IP and both loaded code bytes at CS:+087B.
If CF is clear it sets IP to +08BA; if CF is set it sets IP to +087D.
All other CPU words, all FLAGS and all 1 MiB RAM bytes are preserved.
The branch stops before either successor, including the separate +08BA
LES copy. Unsupported inputs reject without mutation.

Standalone base: `30f8bc45812621c97d5e58adc8d48331bcb81e8f`.
PM-reviewed prior local head: `2b536b3d3eca93eb3f666e2d17cadade6e5324df`.
`deja` found no indexed match for this target.
Six synthetic full-state cases compare all fourteen CPU words and every
RAM byte across both branch directions and varied FLAGS. Wrong loaded
bytes, wrong IP, null pointers and short RAM reject without mutation.
This is a supplied-state gate only; no natural reach, native frame,
full function closure or strict full-C count is claimed.
