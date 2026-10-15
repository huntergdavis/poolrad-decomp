# U3CE RETF 2: directed native stack wrapping

PM i3833 held local head 3c0c43dfabf1de833b11e761aa0843c479576810
because its C return and Unicorn oracle both treated the return IP/CS as a
contiguous four-byte operand. This evidence supersedes that head's return
parity claim at entry SP FFFE and FFFF. A bounded 12-second deja query for
prior native RETF wrap findings expired without results; none were reused.

The source/media identities are pinned in
[the U3CE gate report](U3CE_014E_ZERO_FIRST_GATE.md). The native generator
copies either the exact original final three-byte RETF 2 or the complete
98-byte original body without rewriting instructions. The body is relocated
to a synthetic COM entry; its zero-pointer route accesses no code-relative
data or external call. This is a directed native instruction/zero-path test,
not natural game reach or evidence of heap effects.

## Measured result and repair

Six native executions cross the instruction alone and complete zero path
with entry SP FFFD, FFFE and FFFF. They check all fourteen final CPU words,
24 frame/canary bytes and a landing marker distinguishing wrapped CS from
linear CS. Native uses two stack word reads:

    IP = word(SS, SP)
    CS = word(SS, (uint16_t)(SP + 2))
    SP = (uint16_t)(SP + 6)

Each individual word's second byte remains physically contiguous, including
the word beginning at offset FFFF. Unicorn agrees at FFFD; it differs in
CS alone at FFFE and FFFF for both programs. The previous C gate had the
same CS error on both full-body cases. The repair separates the two stack
word reads. Far-call memory operands retain their contiguous four-byte reader.
Fixtures likewise seed separate stack word offsets, preserving ordinary
memory far-pointer semantics.

The full-state sweep uses the measured native rule only for the divergent
RETF offsets. This explicit correction prevents the emulator bug from
validating the former C bug again. Native observations are independent of
the C implementation; the native comparison also runs the repaired C gate
against the three complete-body captures. Native captures cover the 24
selected frame/canary bytes; the corrected emulator sweep compares all
1 MiB RAM and all fourteen CPU words without a FLAGS mask.

## Native environment and reproduction

DOSBox-X executable SHA-256:
99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
The configuration uses core normal, cputype 386, dummy SDL and a private
bwrap mount with no network. The generator bounds execution to 30 seconds
and log growth to 2 MiB and cleans up its own process group.

An initial harness attempted a MOV word to SS:FFFF and hit the native
segment-limit check before the tested return. That failed setup supplies no
return evidence. The corrected harness seeds a crossing word with byte
stores at SS:FFFF and its physical extension. It does not change CPU type
or the copied original instruction.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u3ce_retf_wrap_native.py

Generated assembly, original instruction copies, COM programs, log and
64-byte capture records stay under ignored
resources/oracle-runs/u3ce-retf-wrap-native. T names the original RETF alone;
G names the complete original zero-path body. Capture SHA-256 values:

| Capture | SHA-256 |
| --- | --- |
| TFFFD.OUT | d35ba3da054a0515f1fdeb06222f5fdb089b0b6196b382d5710a031ad6876338 |
| TFFFE.OUT | ec649dfa18a01ac41142b8d646fd7509d0e967a22c6b013227cc7e70e45acb68 |
| TFFFF.OUT | a41e5e4260db1f4e8fa599de9bb14b45616492b1df2b2e3e073e8f119cc3f1d1 |
| GFFFD.OUT | 2059ee3af7c69a7c2f987f72ca318b3117500562ee8f7dae07e534ba5ea7910d |
| GFFFE.OUT | f2e2a9f05bf2ec626a3b3211beb42c3bb755710ccfa44dd75c93e53e40831fb4 |
| GFFFF.OUT | 1bb436bdb78521b4221392f43dc20004e4a83488ebf831ce4879e0077672684b |

No publication or full-function credit follows from these directed checks.
PM i3883 requires review of the new exact local repair head before publication
and queues separate U6F leaf/helper wrap audits afterward.
