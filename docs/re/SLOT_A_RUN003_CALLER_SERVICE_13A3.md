# Run003 caller through conditional DOS services

This captured-path replay starts from the same-run `3582:348D` CPU and RAM,
executes the observed caller and resolved jump into entry 1, and composes the
existing C entry and resident models. A diagnostic resident entry explicitly
accepts the two guarded, test-only DOS handler profiles from
[SLOT_A_DOS35_COMPOSITION_RUN003.md](SLOT_A_DOS35_COMPOSITION_RUN003.md).
They supply PSP `0814` as a hypothesis, not established process ownership.
The PIT port callback is absent, so execution refuses before the first output
at `10D2:13A3`.

```sh
bash tests/replay/check_slot_a_run003_caller_service_13a3.sh LIBPOOLRAD_RECOVERED_A PRIVATE_RUN003_DIR
```

The script hash-locks both run003 endpoints. The C-produced final state matches
all 14 CPU fields and all 655,360 conventional RAM bytes at 13A3, without
loading intermediate CPU or RAM captures. Upper memory is inaccessible during
the replay. This single captured path assumes no unobserved interrupts; it
establishes neither actual PSP provenance nor PIT state, the 3492 return, or a
complete Up frame. Default resident and port refusal behavior is unchanged.
The separate [caller-to-1386 replay](SLOT_A_RUN003_CALLER_RESIDENT_1386.md)
checks the pre-service boundary; the separately guarded service replay checks
1388, 139E, and 13A0. Prior native boundary observations appear in shared memory
session `01a13101-709d-77e1-9fcb-2a6acbff0046`.
