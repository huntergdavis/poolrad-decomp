#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 1 ]; then echo "usage: $0 PRIVATE_RUN003_DIR" >&2; exit 2; fi
run=$1
root=$(cd "$(dirname "$0")/../.." && pwd)
printf '%s  %s\n' \
 9de586bd5baa6b66dc226178c4ebe5a69b91eb45467f9be7e9ef96fb3e634a88 "$run/RAM-1386.BIN" \
 917b3ad32ce67c6601db787d542d692d96152e51560e47a907db059adab2bf69 "$run/RAM-1388.BIN" \
 4f34ecf7352f2dc2a731e9b4e4df5831e4223b9aabec5341ba4aad4a8ee8e773 "$run/CPU-1386.TXT" \
 0bb1d1b9f7600b2685ae390424995560775037153421adbd1207a537858ff437 "$run/CPU-1388.TXT" \
 31cd7bfd60bc222f1530fed17ceca134f3f5645a6ed9ac84783d276b98ce3525 "$run/RAM-139E.BIN" \
 4315d431b602ddab764ce15dc4aac13a25419a53139a14289c9e44142bed1bf0 "$run/CPU-139E.TXT" \
 606e2012a24646fa66700f6cf89e0d4505f7c2963cdbde67ec15cc7a9212aa40 "$run/RAM-13A0.BIN" \
 418c82068ac7f3f5243272d895926ad2e408cbb7bea2696f7ceb3396bde981b3 "$run/CPU-13A0.TXT" \
 606e2012a24646fa66700f6cf89e0d4505f7c2963cdbde67ec15cc7a9212aa40 "$run/RAM-13A3.BIN" \
 f01232f48666d06d1edafb82b46e2cf0b05d1a67f871f0e81391f9e296470fb7 "$run/CPU-13A3.TXT" | sha256sum -c -
# The retained loaded prefix (not the relocated call) must match its frozen identity.
actual=$(dd if="$run/RAM-13A3.BIN" bs=1 skip=$((0x2e600+0x747)) count=$((0x5a)) status=none | sha256sum | cut -d' ' -f1)
[ "$actual" = 2cd34a43bee8924b156e96d13e0ff84e0536ecf447feeba324ee14648514f3c8 ]
# Pin the loaded call/store, lookup body, and leaf separately.
for spec in 0x131b3:12:e7102f215250c40e24f1944f4f12412f821e5b86d84ad543487963f2cd1a2ab6 0x12f98:177:58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991 0xfc:4:8150b2b9cbc172fc6b6fcd667d4f5dd9f00fd6682b910013b97a62010d4b75f1 0x12f46:82:5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef 0x2eded:5:465d7933dc0ba37aa9d2f63770616100217d517d0ae64de358e9d8fd0dc5cce2 0x08b25:5:9a47b2b664f68837635c35881ffce4ce29631c8479cc83c1c76e21d2e066dee5 0x2edca:35:04310d3d1d925916b9acd10c0caf392ce5931f43ec7e8b150b1b081ce42858f8 0x2edba:16:bd7012dd8784762d374ba7e64fe68fb6da61807be43500bcde803842c39ca413 0x2d6ba:121:2932e53408f1bba96864ff51e310132dda4c72411bd11cd4127c3d09ba010037 0x2eda9:17:6bf0f3887c40ce594007589c9bd0cbe84f960135511494ebf7f17f5608b4d647 0x14868:20:f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7 0x2eda1:8:17a486e5b6bd28bd1bafc4f45f92cfe18bc753d7ce0fc4e750f0f8527d4abe92 0x2d5bb:255:92d75878d963a7f5766b1ea49e2f6c1fb51287775d06a5b6ca485fee8bc43658 0x2d45f:49:ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b; do
  IFS=: read -r offset length digest <<<"$spec"
  actual=$(dd if="$run/RAM-13A3.BIN" bs=1 skip=$((offset)) count="$length" status=none | sha256sum | cut -d' ' -f1)
  [ "$actual" = "$digest" ]
done
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/src/recovered/resident_08ae_vector_137c_cpu.c" "$root/src/recovered/resident_08ae_vector_13b1_cpu.c" "$root/src/recovered/resident_loader_0553.c" "$root/src/recovered/resident_loader_prefix_0338.c" "$root/src/recovered/resident_handler_prefix_02e6.c" "$root/src/recovered/overlay_u3bd_cell_057a_cpu.c" "$root/src/recovered/system_runtime_leaves_cpu.c" "$root/src/recovered/overlay_u3bd_lookup_fullstate.c" "$root/src/recovered/overlay_u3bd_in_grid_031f.c" "$root/src/recovered/overlay_u68_coordinate_writer.c" "$root/src/recovered/overlay_u7d_entry1.c" "$root/src/recovered/overlay_u7d_zero.c" "$root/src/recovered/overlay_u3bd_nibble.c" "$root/src/recovered/overlay_u3bd_cell.c" "$root/src/recovered/resident_08ae_dispatch_00dc_cpu.c" "$root/src/recovered/resident_08ae_table_1742_cpu.c" "$root/src/recovered/resident_08ae_table_17ae_cpu.c" "$root/src/recovered/resident_08ae_delay_006c_cpu.c" "$root/src/recovered/resident_08ae_time_0000_cpu.c" "$root/src/recovered/dos_gettime_00a2_cpu.c" "$root/src/recovered/system_longmul_0a5b_cpu.c" "$root/src/recovered/memory_bus.c" "$root/src/recovered/ega_hw.c" "$root/tests/replay/test_slot_a_dos35_composition_run003.c" -o "$bin"
"$bin" "$run/RAM-1386.BIN" "$run/RAM-1388.BIN" "$run/RAM-139E.BIN" "$run/RAM-13A0.BIN" "$run/RAM-13A3.BIN"
