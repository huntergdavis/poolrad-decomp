# U6F +032F: native RETF wrap repair

PM i3883 assigned this isolated repair from published, unmerged head
25f32f0b09fc68a4eb41e81464f2f20aac4e61df. That head's contiguous four-byte
return and Unicorn validation were unsound at entry SP FFFE/FFFF. The earlier
384 native cases use ordinary stack offsets and do not establish wrap parity.
The separate repair branch is codex/u6f-032f-native-wrap. A bounded 12-second
recall expired without findings; none were reused. The directed capture
method reuses the U3CE native-wrap generator at approved head a9ab3aa, with
U6F-specific source, inputs, cleanup size and output regions.

The exact media, 327-byte/121-start original extent, RETF 8 at +0473 and
source SHA-256 are independently pinned by the existing audit and listed in
[the leaf report](U6F_VECTOR_MERGE_032F.md). Fifteen directed native frames
execute either the exact original RETF 8 alone or the entire original body
in each mode 0/1/2/3, at entry SP FFFD/FFFE/FFFF. Mode 3 covers the skipped
vector route; modes 0/2 cover interleaved OR/AND and mode 1 covers separate
OR and AND loops. Positive vector counts are five and six respectively.

Native checks all fourteen final CPU words without a FLAGS mask, a landing
marker distinguishing wrapped CS from linear CS, and 229 selected RAM bytes:
48 stack/frame/physical-extension bytes, two 55-byte records, two 32-byte
vectors, and seven mode/global-pointer/count bytes. The crossing word is
seeded with byte stores to avoid a setup-time native MOV segment-limit fault.
The copied body executes at COM +1100 with relative branches unchanged.
No original byte array or generated COM is committed.

Native establishes IP=word(SS,SP), CS=word(SS,(uint16_t)(SP+2)), then
SP=(uint16_t)(SP+12). Individual words remain physically contiguous.
All FFFD frames agree with Unicorn. At FFFE/FFFF, Unicorn differs only in CS
in all five program variants (ten divergences), and the old C differs only
in CS in all four full-body modes (eight mismatches). Repaired C matches
all twelve complete-body native frames, including RAM. Memory far-pointer
reads retain their separate contiguous operand semantics.

The corrected 425-case sweep compares all 1 MiB RAM and all fourteen CPU
words, explicitly substituting the measured native RETF rule at the two
Unicorn divergence offsets. It visits 121/121 starts and 15/18 conditional
outcomes; three unsigned-zero JA outcomes are impossible. Focused strict
C11/CTest and ASan/UBSan cover the new wrap regressions. Synthetic native
frames do not establish natural reach, source ownership or full-C credit.

DOSBox-X SHA-256 is
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3,
core normal, cputype 386, dummy SDL, isolated bwrap with no network. Execution
is bounded to 30 seconds and 2 MiB of log growth, with owned-group cleanup.
Private artifacts stay under ignored resources/oracle-runs/u6f-032f-retf-wrap-native.
Reproduce with:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6f_032f_retf_wrap_native.py

Each capture contains 267 bytes. SHA-256 values:

| Capture | SHA-256 |
| --- | --- |
| G0FFFD.OUT | 4e4bb9c4fae97b2c387c4bb17499986e6cef231126737026b87f15af3c243381 |
| G0FFFE.OUT | 2e87a026dc1c15719405db768f6121eef1d7661e47a5ba5ba46e1795f620c0a4 |
| G0FFFF.OUT | 4068293d298f0dc6c8e093ba585d3026ad66ea1a897db90c252a4cf90f4c16fe |
| G1FFFD.OUT | 1f23e1042b96b82f1844dbd8de500f5aa5d28747fcb9b7f937a9f3a3bc98fcf3 |
| G1FFFE.OUT | bfb8147d3b892d77fbc168bde7391b32b9b4cc70d8f22c104c9d308423c23ad0 |
| G1FFFF.OUT | 3b15ae9e329b1fe6afd53c2605e469369f4690ac02cdf5b8c29bf2355948fd35 |
| G2FFFD.OUT | 10f4e3a68bc8acdfc94b674f5c24492f18f0b93636dc19605d712f3a0720aad7 |
| G2FFFE.OUT | 3abca120277e99e33bce3d43fb39efc66bd723f7ecbc5867445f65a018e4bdd7 |
| G2FFFF.OUT | bf858bfad0aba00acb204e42f3a38206452c9bad199689bc0287a38cc7e42862 |
| G3FFFD.OUT | c51f39ca31d58eddc43f92758ddc6a3f9470698d245dc121ebdedc6b8da67bdf |
| G3FFFE.OUT | 52eaafec4c7590ffac2b6d4f5273a6c75c146fe11a773b1a79f027c4a0f5c6f8 |
| G3FFFF.OUT | 5224812963a18ff9af49eaace099d4e043e0b28c54876424a62114ef0e2e19bd |
| T3FFFD.OUT | 150d1695fb266c65956e793443ddc1b5ba1852cc2005c0fc9dffee13ee5e117a |
| T3FFFE.OUT | c9c490b634e877aab020c2bb3e0b82e5c3fbed70aaa1b6f9e092232cb04ffe57 |
| T3FFFF.OUT | b9076eb1b0f94064c2514ccc5ee72bbc1d0df7fc13c2a862c5a362090c55e56d |
