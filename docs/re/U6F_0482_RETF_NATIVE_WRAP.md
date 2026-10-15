# U6F +0482: closed-helper native RETF repair

PM i3883/i3989 assigned this separate branch from published head
58fac09a12be7836c60a0366d9dc8fd1a9c5a69c, named
codex/u6f-0482-native-wrap. Bounded 12-second recall expired without results;
none were reused. The directed native harness adapts the U6F leaf repair
at head 45314d7179e53ca5ccd1a882623bfce5319c6d6d. Its helper-specific inputs,
cleanup immediates and capture regions are independently constructed.

All four original source extents and hashes are rechecked by
[the prefix audit](U6F_PREFIX_0482.md). Native executes each complete original
helper unchanged at COM +3100 and separately its own final RETF instruction:

| Helper | Bytes | Cleanup | Complete-body variants | Native frames |
| --- | ---: | ---: | --- | ---: |
| 0802:094A selector | 19 | 2 | save/replace selector | 6 |
| SYSTEM 0AF8:0B0B load | 26 | 4 | empty / five-byte string | 9 |
| SYSTEM 0AF8:0C3B character | 18 | 2 | one-character string | 6 |
| SYSTEM 0AF8:0B98 append | 44 | 4 | length 3+5 / saturated 250+10 | 9 |

Each variant crosses helper entry SP FFFD/FFFE/FFFF. The four helpers were
audited sequentially before repair and rerun sequentially after repair.
Thirty frames comprise twelve original-instruction-only frames and eighteen
complete-helper frames. Every capture compares all fourteen CPU words with
no FLAGS mask, a wrapped-versus-linear return landing marker, and 562 selected
RAM bytes: 48 stack/frame/extension bytes, two 256-byte string buffers and
two selector bytes. A temporary test wrapper calls the actual static C helper
without exposing a new product API. Original copies, generated COM programs,
wrappers, logs and captures remain ignored under resources/.

Native establishes separate word reads at SS:SP and SS:(uint16_t)(SP+2),
then SP=(uint16_t)(SP+4+cleanup). Each individual word remains physically
contiguous. At FFFD, Unicorn agrees. At FFFE/FFFF it differs only in CS:
twenty reported divergences across all variants. The former C helper return
had twelve CS-only complete-body mismatches; repaired C has none. All
selected memory bytes match. The generic memory far-pointer reader is
unchanged, because its four-byte operand remains physically contiguous.

The corrected integrated prefix sweep passes 681 full-state cases, comparing
all 1 MiB RAM and all fourteen CPU words: 680 callback stops, one genuine
alias-altered return, 32/32 caller starts and 47/47 helper starts. Its oracle
explicitly uses the native-established RETF correction at the two divergent
SP values. Seven former apparent control stops disappear after this repair.
Twelve separate U3CE source-boundary probes remain unchanged.

Strict C11 Werror, ASan/UBSan (sandbox leak detection disabled), and focused
CTest pass. New C cases place each helper return at FFFD/FFFE/FFFF through
the prefix. An initial test expected all destination far operands to retain
SS even when starting at FFFF; the contiguous LES operand instead reads its
segment from the physical extension. The corrected synthetic fixture checks
that distinct behavior explicitly while retaining wrapped RETF word offsets.

DOSBox-X SHA-256:
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
Core normal, cputype 386, dummy SDL, private bwrap mount with no network;
each run is bounded to 30 seconds and 2 MiB log growth with owned-group
cleanup. These are supplied synthetic helper frames, not native caller
reach, natural gameplay, full native 1 MiB captures or unresolved call effects.
No ownership or full-C count credit follows.

Reproduce each helper separately:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6f_0482_helper_wrap_native.py --helper selector

Repeat with strload, charstr and strcat16. Private 600-byte capture hashes:

| Helper/capture | SHA-256 |
| --- | --- |
| selector/G0FFFD.OUT | 4b3d4e2472b85f715e21072792475a8d01e3123c457571b58a8461fe5f7809f0 |
| selector/G0FFFE.OUT | bfc76bae369efd7ed0970a4a9522eb49337ef1f72e472958ee5c5639fc4c9270 |
| selector/G0FFFF.OUT | a3b4c54b900368e5645b37d3610b0fd984ebd1192125cf914c6101531539cdc2 |
| selector/T0FFFD.OUT | 04dcb1b7b4282a8a2c411f4ed7078b53c344b4b5a9af03cf375e1c38883bd136 |
| selector/T0FFFE.OUT | 65dd967daa84efb839564110aa5f508e53af5ae8b103724efc350b4904a5541d |
| selector/T0FFFF.OUT | aa63d5f20b10c9bcf1a47d157afe99f019a40e9460d80d0eff9b61eadef84c55 |
| strload/G0FFFD.OUT | b53c959174d4b22d90bfd8c624d0a9c07ffc46c40aba453df69fde8c32b224da |
| strload/G0FFFE.OUT | 8cef1db9613f7b8013f1011cd0b2ea6bef94aa2544a745c1635d222773bef6ef |
| strload/G0FFFF.OUT | b5d0da0a833241eb6de20443544677395964cfd6501e12da1b97c86013c0a8f8 |
| strload/G2FFFD.OUT | 049aedd5934bec0d0c405d2d9d813518526381664f56d2a44058217941e996c5 |
| strload/G2FFFE.OUT | c5eb74a19ffe407e7897eb5a9bd5887d6777b0bdee3af841988a7c71b4fdb840 |
| strload/G2FFFF.OUT | d0cdcec1569fdd3cefc35d54e6a757a0a83bb462f93abd15c6ad187b0484465c |
| strload/T0FFFD.OUT | e6086429f7b4f93b0fc6d48cb8037eea840ddcbb1dff16aee48d90cb7e7795d6 |
| strload/T0FFFE.OUT | 0c566397cb11a762736b9996594dbf2006dd8ce0ba75da8277e50a7db738a8f0 |
| strload/T0FFFF.OUT | 826c4bc1f373a0dc3a6dd4926fb7dbc152858cf152f101f17974a51ee9ee6827 |
| charstr/G0FFFD.OUT | 451e6a67573a146f56a490cd1ca2f5cda060b1ea22024ce2ecb4862b0ac49292 |
| charstr/G0FFFE.OUT | db051b72741efc9e4aee09a9619948edc177236a9bda5af3196fbe0e4a2544e0 |
| charstr/G0FFFF.OUT | 6fa767ed56bee33aac9c76c23a23fc1032c55a818c10debde46e8c707a0b46e6 |
| charstr/T0FFFD.OUT | 04b013c0c6bc73dd08f3dbfb77a82f6a727e7b8f5523a47257579162dfd31204 |
| charstr/T0FFFE.OUT | 4aa2c4442e1bdaa1040e56226f4dae8960c81efa3595f45ced191192c3797edb |
| charstr/T0FFFF.OUT | a9921718127f5f98c13b24d5d00c41c510ae083b10b81bbb5441ae7813a74c94 |
| strcat16/G0FFFD.OUT | 476b4338afacade3f548cb2fd07c50b61df37f102a6f800ddfc3eddc9cb28a2b |
| strcat16/G0FFFE.OUT | 9f888c3ab6f7741d6a903a41168aa079b576a3fee882e1af662554360f95a2c4 |
| strcat16/G0FFFF.OUT | 6eb99d26186bc7255917663c7d1ab63d1517b7d7de619e094d63343fe66b0c0b |
| strcat16/G1FFFD.OUT | fbdf560e5973cc428ef125a1eef25b8457fb496078b4f579dff2a4b13819473e |
| strcat16/G1FFFE.OUT | d65ccd895b0bb13ff54cd0fbad751c06c5d892b2c1aa26c4cfe84b57b0dfb3aa |
| strcat16/G1FFFF.OUT | 4eb71b27f024fc84ee58854b603f336b169b06a3c857c985beb6baa1cf3ae63f |
| strcat16/T0FFFD.OUT | e6086429f7b4f93b0fc6d48cb8037eea840ddcbb1dff16aee48d90cb7e7795d6 |
| strcat16/T0FFFE.OUT | 0c566397cb11a762736b9996594dbf2006dd8ce0ba75da8277e50a7db738a8f0 |
| strcat16/T0FFFF.OUT | 826c4bc1f373a0dc3a6dd4926fb7dbc152858cf152f101f17974a51ee9ee6827 |
