#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 1 ]]; then
    echo "usage: $0 PRIVATE_RUN_t4492-slot4-logc-030_DIR" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
run=$(realpath -e -- "$1")
case "$run" in
    "$resources"/*) ;;
    *) echo "private inputs must stay in this tree's ignored resources/" >&2; exit 2 ;;
esac

# Anchor images and CPU transcriptions (the audit locks the traces as well).
printf '%s  %s\n' \
 dbbc1e3e4abd68aad0de729be534bace84ebb6c8cd074a4263096af194f73b37 "$run/RAM-10D2-00F4.BIN" \
 b227e2c1fadb3b1b8d3dd00a1020d83edddcdc193e16acbe26c93f181c5890c8 "$run/RAM-10D2-0165.BIN" \
 342882515c7dd4f852a17a15bc78c4d7bf147b2cb287e3e7169abbcaee02e270 "$run/RAM-10D2-13C6.BIN" \
 99d632bb8349102c6f41a495ce212367e07a4d35b32206b9654703e2e065e270 "$run/RAM-2E60-0747.BIN" \
 a1be6e91ab4f9386a3c495cfdb3c776e2758f6e802de20c56b30877918b301bb "$run/CPU-10D2-00F4.TXT" \
 be674fba164d5b5bde0407d34d6cfd3f6f4515a05247c80bd227a2c5d3f73218 "$run/CPU-10D2-0165.TXT" \
 c5403b94eb3b242d82c7646dff8c566bcd54412e02af6e3d4e0112c97bf77c35 "$run/CPU-10D2-13C6.TXT" \
 c4b279854701fd67463ae3f7cb58a1d9eb805c21891786d10175bddbcfa56396 "$run/CPU-2E60-0747.TXT" | sha256sum -c -

build_dir="$resources/build-slot-a-measured-030"
mkdir -p "$build_dir"
s="$repo_dir/src/recovered"
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -I"$repo_dir/include" \
    "$s/resident_08ae_vector_13b1_cpu.c" "$s/resident_08ae_dispatch_00dc_cpu.c" \
    "$s/resident_08ae_table_1742_cpu.c" "$s/resident_08ae_table_17ae_cpu.c" \
    "$s/resident_08ae_delay_006c_cpu.c" "$s/resident_08ae_time_0000_cpu.c" \
    "$s/dos_gettime_00a2_cpu.c" "$s/system_longmul_0a5b_cpu.c" "$s/memory_bus.c" "$s/ega_hw.c" \
    "$repo_dir/tests/replay/test_slot_a_measured_030.c" -o "$build_dir/test_slot_a_measured_030"
"$build_dir/test_slot_a_measured_030" \
    "$run/RAM-10D2-00F4.BIN" "$run/CPU-10D2-00F4.TXT" "$run/RAM-10D2-0165.BIN" "$run/CPU-10D2-0165.TXT" \
    "$run/RAM-10D2-13C6.BIN" "$run/CPU-10D2-13C6.TXT" "$run/RAM-2E60-0747.BIN" "$run/CPU-2E60-0747.TXT" \
    "$build_dir/stage1-residue.txt"
nice -n 15 python3 "$repo_dir/tests/replay/audit_slot_a_logl_030.py" "$run" "$build_dir/stage1-residue.txt"
