#!/usr/bin/env python3
"""Check the source route and prior loaded mapping for a future U19 stop.

Only ignored originals and an existing post-printer RAM capture are read.
The public JSON is a capture template, never an observed same-run manifest.
"""

import argparse
import json
from pathlib import Path
import struct

from cold_start_codeword_row14_boundary_audit import (
    audit_original, digest, far_target, require,
)
from slot_a_printer_page_audit import unpack


U19_FILE = 0x09AC
U19_LIVE = 0x38D30
RESIDENT_LIVE = 0x08240
STUB_LIVE = 0x08400
LOAD_BASE = 0x0824
POST_RAM_SHA = "84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699"
ORIGINAL_ROUTE_SHA = "f610b645b80150a4667efb87802cee3a76531bc7791dd02f75bd501ca93432d2"
LOADED_ROUTE_SHA = "52f36b7575cbc5009ea1c9a597a0730b29cda4d7931073eedc5ab2fa4103595a"
ORIGINAL_START_CALL_SHA = "54e957158779fadaefe40e9504e2ea47cb99c7425ee051e8c2c849f2940596c7"
LOADED_START_CALL_SHA = "8eae8facca4e09951d8568742355469f03771c781568754c37883c332fdb9e6e"
LOADED_STUB_SHA = "bf0c36dda99d8b3d11ab4b874d52757a90e1186a03e3fc7271d704c26634a052"
ENTRY_GATE_SHA = "b998e4d2d6493b9a8c1352791d6ebc9293378c016e9a6298ccf15c9d4ae89d4c"
GLYPH_BRANCH_SHA = "5abff77204a4fa392ef972f789d1e57a84495068a8e03aeb6d694c24c825ad3d"
FINAL_SETUP_SHA = "47736a813d62c3d22ee798530485492669d834d20c4a2d448a8cb9bc33914a75"
FINAL_SETUP_LOADED_SHA = "ca56b7ab378d34e6a576559613f2784044bca4e02bd2795151f2763b0ab711b6"
CALL_SITES = (
    0x0111, 0x0135, 0x013A, 0x0152, 0x0157, 0x016F, 0x0174, 0x018C,
    0x0191, 0x01BB, 0x01E5, 0x01FD, 0x0202, 0x022C, 0x0269, 0x0281,
    0x0286, 0x029F, 0x02B6, 0x02BB, 0x02D3, 0x02D8,
)
STOP_IDS = (
    "credits_return", "u19_stub", "u19_entry", "u19_pre_printer",
    "printer_entry", "selected_witness_store", "store_successor",
    "u19_post_printer",
)


def short_target(data: bytes, base: int, site: int) -> int:
    displacement = struct.unpack_from("<b", data, base + site + 1)[0]
    return (site + 2 + displacement) & 0xFFFF


def audit(exe: bytes, ovr: bytes, ram: bytes, template: object) -> None:
    audit_original(exe, ovr)
    require(len(ram) == 655360 and digest(ram) == POST_RAM_SHA,
            "retained post-printer RAM differs")
    image, *_ = unpack(exe)
    original = ovr[U19_FILE + 0x00FD:U19_FILE + 0x02DD]
    loaded = ram[U19_LIVE + 0x00FD:U19_LIVE + 0x02DD]
    require(len(original) == len(loaded) == 480 and
            digest(original) == ORIGINAL_ROUTE_SHA and
            digest(loaded) == LOADED_ROUTE_SHA,
            "U19 entry-to-printer original/loaded route differs")
    require(digest(ovr[U19_FILE + 0x00FD:U19_FILE + 0x0111]) ==
            ENTRY_GATE_SHA and
            digest(ovr[U19_FILE + 0x023D:U19_FILE + 0x026E]) ==
            GLYPH_BRANCH_SHA and
            short_target(ovr, U19_FILE, 0x0108) == 0x010D and
            short_target(ovr, U19_FILE, 0x0242) == 0x026E,
            "U19 branch successors differ")
    require(digest(ovr[U19_FILE + 0x02C0:U19_FILE + 0x02DD]) ==
            FINAL_SETUP_SHA and
            digest(ram[U19_LIVE + 0x02C0:U19_LIVE + 0x02DD]) ==
            FINAL_SETUP_LOADED_SHA,
            "row-14 final setup differs")

    expected_changes = {site + byte for site in CALL_SITES for byte in (3, 4)}
    actual_changes = {at for at in range(0x00FD, 0x02DD)
                      if ovr[U19_FILE + at] != ram[U19_LIVE + at]}
    require(actual_changes == expected_changes,
            "U19 loaded differences exceed 22 far-call segment relocations")
    for site in CALL_SITES:
        require(ovr[U19_FILE + site] == 0x9A and
                ram[U19_LIVE + site] == 0x9A,
                f"far-call opcode differs at +{site:04X}")
        original_target = far_target(ovr, U19_FILE + site)
        loaded_target = far_target(ram, U19_LIVE + site)
        require(loaded_target == (original_target[0],
                                  (original_target[1] + LOAD_BASE) & 0xFFFF),
                f"far-call target differs at +{site:04X}")

    start_original = image[0x0122:0x0127]
    start_loaded = ram[RESIDENT_LIVE + 0x0122:RESIDENT_LIVE + 0x0127]
    stub = ram[STUB_LIVE + 0x0025:STUB_LIVE + 0x002A]
    require(digest(start_original) == ORIGINAL_START_CALL_SHA and
            digest(start_loaded) == LOADED_START_CALL_SHA and
            digest(stub) == LOADED_STUB_SHA and
            far_target(image, 0x0122) == (0x0025, 0x001C) and
            far_target(ram, RESIDENT_LIVE + 0x0122) == (0x0025, 0x0840) and
            stub[0] == 0xEA and
            struct.unpack_from("<HH", stub, 1) == (0x00FD, 0x38D3) and
            far_target(ovr, U19_FILE + 0x02D8) == (0x0626, 0x0709) and
            far_target(ram, U19_LIVE + 0x02D8) == (0x0626, 0x0F2D),
            "retained resident/stub/printer mapping differs")

    require(isinstance(template, dict) and template.get("version") == 1 and
            template.get("status") == "SOURCE_ONLY_TEMPLATE_NO_NATIVE" and
            template.get("candidate_hit_cap") == 128 and
            template.get("witness") == {"segment": "A000", "offset": "118F",
                                        "bit": "20"},
            "public stop-template contract differs")
    stops = template.get("ordered_stops")
    require(isinstance(stops, list) and
            tuple(stop.get("id") for stop in stops if isinstance(stop, dict)) ==
            STOP_IDS and len(stops) == len(STOP_IDS),
            "public stop-template order differs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--ram", type=Path, required=True)
    parser.add_argument("--template", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe.read_bytes(), args.ovr.read_bytes(), args.ram.read_bytes(),
          json.loads(args.template.read_text()))
    print("U19 source/loaded mapping: 22 far calls, 44 relocation bytes, 0 other changes")
    print("eight-stop public template checked; pre-02D8 and selected plane pair missing")
    print("no new native route, first writer, page, RGB, or C parity claim")


if __name__ == "__main__":
    main()
