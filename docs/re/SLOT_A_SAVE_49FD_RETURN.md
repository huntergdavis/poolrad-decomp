# SAVE return after 49FD store

The retained-script audit identifies destination 49FD; the shared-state
fixture supplies source value 5 synthetically. After the bounded class-0 store,
the fixture reuses the existing SAVE epilogue and dispatcher-return adapters
through runner 31A8. Their caller-authenticated references, immutable-code,
disjoint-host and live-frame contracts remain in force.

The chain checks the returned frame, preservation of the synthetic window word,
DS:84E8 flag and earlier SAVE effect, plus epilogue-reference refusal. It does
not establish the actual source value, same-invocation provenance, a complete
Up route or native 345B parity. The next runner iteration is not executed.
