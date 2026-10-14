# Unit 0x7F +05E0: bounded native-reach follow-up

This follow-up does **not** promote PR #387's provisional party-count routine
to strict full C or verified game identity. Its complete original-byte/C
translation is already in [U7F_PARTY_COUNT_05E0.md](U7F_PARTY_COUNT_05E0.md).
The remaining gate is an identity-confirmed native entry/return and comparison
with that translation. No such pair was obtained in this bounded pass.

## Reused and checked evidence

The resumed assignment began at main
`75d24676525ad0a984e66cd14764de87638d96f2`, including merged PR #387
(`2dea9958b002d9c1a59fb194795b320dd1428217`). A required bounded
`timeout 20s deja "poolrad u7f 05e0 PR387 native frame strict parity"`
attempt first encountered the sandbox index-lock restriction; its escalated
retry timed out without a result. No session material was reused.

The original overlay and caller information were reused from the linked note
and independently decoded. The body remains file `[0x25269,0x252C3)`,
90 bytes / 30 instructions, SHA-256
`d4e73837529d5f4bb1ac788885f216ba39ccf5f67778a8278862f846c752a969`.
Entry 3 at `+063A` calls the leaf at `+0652`, storing AL at `+0655`.
The three callers select public stub `008F:002F` on key AL=`53`:
unit `0x1E:+0DF4`, `0x20:+106A`, and `0x23:+0663`.
The preceding Pool branches target entry 2, so they cannot substitute for
reaching the Share branch. Code offsets from different overlay units must
not be identified by numerical coincidence.

## Private bounded run

The ignored run directory was
`resources/oracle-runs/u7f-05e0-share-fullc-001`. Original inputs:

| Input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The debugger ran under a private X display and Bubblewrap, with networking
disabled and writes restricted to the ignored copied run. An outer
`timeout --signal=TERM --kill-after=5s 600s` bounded its lifetime. Startup
queued `AUTOTYPE -w 28 -p 2 enter l a` and `DEBUGBOX START`.
The initial debugger record was `CS:IP=1796:0012`, `DS=ES=0814`,
`SS:SP=193E:0080`, `BP=091C`, `AX=0000`. The PSP-derived image load base
is `0824`; public entry-3 stub relocation is therefore `08B3:002F`.
The debugger confirmed that breakpoint was armed, then execution resumed.

The run ended with timeout status 124. It produced no entry-3 breakpoint
stop and no leaf entry/return RAM or CPU pair. No verified Share-menu state
was captured; the startup/autotype sequence alone does not establish the
saved route or prove that Share was selected. In particular, this attempt
does not reproduce all the menu exploration described by the earlier PR.
No game instructions, script bytes, or guest registers were edited.

## Exact blocker and next gate

The missing evidence is a real call through entry 3 and its dynamically
loaded, hash-verified unit-0x7F body, followed by the `+05E0` entry/return
CPU and RAM pair. A relocated-stub breakpoint and captured party data with
a synthetic frame do not supply that evidence. The timeout is a bounded
route/instrumentation limit, not proof of unreachability or a C defect.

Per the assignment, further exploration was stopped and work moved to the
disjoint unit `0x6A:+0824` follow-on from PR #391. Neither this note nor the
scope switch advances any routine count; PM and independent review retain
the promotion decision.
