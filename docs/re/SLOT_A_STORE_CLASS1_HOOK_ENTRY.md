# Synthetic AND class-1 store and hook entry

The class-1 continuation in `U25_STORE_WORD_0C77.md` writes the window then
pushes the destination, value and far return frame for hook `0A74`. This
bounded resume stops at hook entry; the hook itself and store-helper return
are unexecuted. It rejects frame, pointer, code and destination aliases that
could affect its modeled sequence. Evidence also includes Dunking Sheep memory
session `3e3ff494-f865-446a-8777-6a4baca8de94`.

The source byte `95`, prior IF predicate and value 11 are synthetic. This is
not same-invocation source provenance, native `345B` parity, or a complete Up
route. The caller supplies immutable code and disjoint host objects.
