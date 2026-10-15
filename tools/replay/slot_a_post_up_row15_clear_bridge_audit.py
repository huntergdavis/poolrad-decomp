#!/usr/bin/env python3
"""Hash-locked audit of two post-Up printers and their row-15 clear bridge.

All media, private text/font, RAM, and EGA payloads remain in resources/.
Only addresses, hashes, and counts are printed.
"""

import argparse
from pathlib import Path
import struct
import subprocess

from slot_a_printer_page_audit import (
    FONT_SHA, PRINTER_IMAGE_SHA, PRINTER_RUNTIME_SHA, START_SHA,
    check_hash, unpack, word,
)

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457"

RAM_SHA = {
    "first-printer-entry": "7b3ef058c858962c92e42bcf4dbbae341dab58b93d1821f71532777ef07bdad8",
    "first-printer-return": "8c11527ea071fa4aea4972f5d9b8d2dae915058d7f0e75c627e41485d2d17a70",
    "second-printer-entry": "f6015203f697b7c8007029d4b629a6c3154a9fae85938b958896b5af0c093719",
    "second-printer-return": "1c805a0f0c35e2c6f6b4d7c70318f296a95cc2f1289f05c1123df23a78972d7f",
    "row15-clear-entry": "cc4e20fd5a91cb158aa09239924ab1d3cfd83cc53e3209f4a29917f0c3a89475",
    "row15-first-store": "05e5cad6daff48a72a57c507a464258b483f3b1209eb6d95d00b9bd7822d36b0",
    "row15-clear-return": "438cf267bba6666971cac6dba404e8d1cbbaee18e1496702423d623c20427714",
}
PAGE_SHA = {
    "first-printer-entry": "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44",
    "first-printer-return": "a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac",
    "second-printer-entry": "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44",
    "second-printer-return": "1019a688b324afa165f731013c6491c0cb54232262403f7c70cbb7fb97543773",
    "row15-clear-entry": "a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac",
    "row15-clear-return": "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44",
}
EMPTY_PLANES = (
    "15271c33fcee318327fd238aa423b9317410f61c217bac55af2bbf21cd16e1c4",
    "e85f6e37afae7a09aa580c274642222aeb071415f7e14e98af5cace31b7ee64f",
    "f96d5f3a995943f4e58f70f03d72ab44714ad8b2cd0987f01eb5f1f4fe28dcce",
    "d34da7cd6a7af447acdd93f48a289c4646b1926fe24ac41d0dc175ac709d3c84",
)
FIRST_PLANES = (
    EMPTY_PLANES[0],
    "f64094c6eae3e506c8c0217b55d092013aed86040a6903a23e644bc5c3f3979e",
    EMPTY_PLANES[2],
    "0f52c2fc90ea411ec434d06e18360d14b671aa0dbf1b3820126c2dbd3c10e746",
)
SECOND_PLANES = (
    EMPTY_PLANES[0],
    "34b3a033856243e4ec4267a0b3d01ba9606ddd78ef753bdcaca2ec57d75e5df2",
    EMPTY_PLANES[2],
    "2c33999d2dbb500ffeaaa055f05f462a83ec791565075206fdda6cf4e4576bf8",
)
PLANE_SHA = {
    "first-printer-entry": EMPTY_PLANES,
    "first-printer-return": FIRST_PLANES,
    "second-printer-entry": EMPTY_PLANES,
    "second-printer-return": SECOND_PLANES,
    "row15-clear-entry": FIRST_PLANES,
    "row15-clear-return": EMPTY_PLANES,
}
SOURCE_SHA = (
    "e767d5c8ef552436c5e411af72d8eca9581f3feacbb988c67c16bf2b701bc2fe",
    "f13e024bc34f688552e96d6cfb8be870b68d2ada58f4ae6fdea7fd7e583e9860",
)
PRINTER_ORIGINAL_CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
PRINTER_LOADED_CALL_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
RECT_ORIGINAL_BODY_SHA = "f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5"
RECT_LOADED_BODY_SHA = "02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f"
RECT_ORIGINAL_CALL_SHA = "2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c"
RECT_LOADED_CALL_SHA = "8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2"
PARENT_ORIGINAL_CALL_SHA = "395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f"
PARENT_LOADED_CALL_SHA = "2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850"
FIRST_STORE_SHA = "50b2d3be54fb620b49268d3b4bd0c759531794c4c735fc2916071807cf9ee7a6"
RECT_FRAME = (0x01F6, 0x0C15, 0, 0, 15, 38, 15, 17)
FIRST_STORE_FRAME = (0x0145, 0x0F2D, 0, 22, 0x12D1, 0xA000)


def validate_rect_frame(ram: bytes) -> None:
    if len(ram) != 0xA0000:
        raise ValueError("rectangle entry RAM size changed")
    if struct.unpack_from("<8H", ram, 0x1D160 + 0x3B62) != RECT_FRAME:
        raise ValueError("row-15 rectangle frame changed")
    if struct.unpack_from("<2H", ram, 0x1D160 + 0x3B80) != (
            0x2B47, 0x08DE):
        raise ValueError("rectangle parent return changed")
    if ram[0x14A00 + 0x4A1A] != 0:
        raise ValueError("active EGA page changed")


def validate_delta(before: bytes, after: bytes) -> int:
    if len(before) != 32000 or len(after) != 32000:
        raise ValueError("four-plane page size changed")
    changed = {index for index, (a, b) in enumerate(zip(before, after))
               if a != b}
    allowed = {lane * 8000 + line * 40 + col
               for lane in (1, 3) for line in range(120, 128)
               for col in range(17, 36)}
    if len(changed) != 210 or not changed <= allowed:
        raise ValueError("post-Up row-15 page delta changed")
    if sum(8000 <= i < 16000 for i in changed) != 105 or \
            sum(24000 <= i < 32000 for i in changed) != 105:
        raise ValueError("post-Up plane delta distribution changed")
    return len(changed)


def native_route_fields(memory: bytes) -> tuple[tuple[int, ...], tuple[int, ...]]:
    if len(memory) != 0xA0000:
        raise ValueError("native route RAM size changed")
    ds = 0x14A00
    if struct.unpack_from("<2H", memory, ds + 0x49D2) != (0, 0x3996):
        raise ValueError("window-zero far pointer changed")
    party = tuple(memory[ds + offset] for offset in (0x6AAD, 0x6AAE, 0x6AAF))
    words = struct.unpack_from("<3H", memory, 0x39960 + 0x18E)
    return party, words


def ram(run: Path, phase: str) -> bytes:
    result = (run / f"{phase}-private.ram").read_bytes()
    if len(result) != 0xA0000:
        raise ValueError(f"{phase} RAM size changed")
    check_hash(result, RAM_SHA[phase], f"{phase} RAM")
    return result


def page(run: Path, phase: str) -> bytes:
    planes = []
    for lane, digest in enumerate(PLANE_SHA[phase]):
        data = (run / f"{phase}-plane{lane}-private.bin").read_bytes()
        if len(data) != 8000:
            raise ValueError(f"{phase} plane {lane} size changed")
        check_hash(data, digest, f"{phase} plane {lane}")
        planes.append(data)
    result = b"".join(planes)
    check_hash(result, PAGE_SHA[phase], f"{phase} full page")
    return result


def far_call(data: bytes, digest: str, target: tuple[int, int], label: str) -> None:
    check_hash(data, digest, label)
    if len(data) != 5 or data[0] != 0x9A or \
            struct.unpack_from("<HH", data, 1) != target:
        raise ValueError(f"{label} target changed")


def printer_input(entry: bytes, sp: int, source_sha: str, before: bytes) -> bytes:
    frame = struct.unpack_from("<7H", entry, 0x1D160 + sp)
    ret, ret_seg, off, seg, color, row, col = frame
    if (ret, ret_seg, color, row, col) != (
            0x2B5A, 0x08DE, 0xA00A, 0xA00F, 0xA011):
        raise ValueError("post-Up printer frame changed")
    source_at = seg * 16 + off
    length = entry[source_at]
    if length != 19:
        raise ValueError("post-Up printer source length changed")
    check_hash(entry[source_at:source_at + length + 1], source_sha,
               "private post-Up printer source")
    ds = 0x14A00
    font_off, font_seg = struct.unpack_from("<2H", entry, ds + 0x5E20)
    if (font_off, font_seg) != (0, 0x3CB6):
        raise ValueError("post-Up font pointer changed")
    font = entry[font_seg * 16 + font_off:font_seg * 16 + font_off + 1416]
    check_hash(font, FONT_SHA, "live post-Up font")
    if (word(entry, ds + 0x4A16), word(entry, ds + 0x4A18),
            entry[ds + 0x4A1E]) != (0xA000, 0xA000, 1):
        raise ValueError("post-Up EGA page mode changed")
    return (font + bytes((length, col & 255, row & 255, color & 255)) +
            entry[source_at + 1:source_at + length + 1] +
            bytes(255 - length) + before)


def run_probe(probe: Path, input_path: Path, output_path: Path,
              data: bytes, extra: tuple[str, ...], report: str) -> bytes:
    input_path.write_bytes(data)
    result = subprocess.run([str(probe.resolve()), str(input_path),
                             str(output_path), *extra], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != report:
        raise ValueError(f"C probe report changed: {result.stdout!r}")
    return output_path.read_bytes()


def audit(exe_path: Path, emulator_path: Path, printer_run: Path,
          watch_run: Path, printer_probe: Path, rect_probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash(emulator_path.read_bytes(), EMULATOR_SHA, "heavy DOSBox-X")
    for run in (printer_run, watch_run):
        check_hash((run / "POOLRAD/START.EXE").read_bytes(), START_SHA,
                   "run START.EXE")
        check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), GAME_SHA,
                   "run GAME.OVR")
        check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), SAVE_SHA,
                   "run slot-A save")
    image, *_ = unpack(exe)
    far_call(image[0x36F5:0x36FA], PRINTER_ORIGINAL_CALL_SHA,
             (0x0626, 0x0709), "original printer call")
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA,
               "original printer body")
    far_call(image[0x36E2:0x36E7], PARENT_ORIGINAL_CALL_SHA,
             (0x0137, 0x03F1), "original parent call")
    far_call(image[0x4101:0x4106], RECT_ORIGINAL_CALL_SHA,
             (0x00B7, 0x0709), "original rectangle call")
    check_hash(image[0x7147:0x7201], RECT_ORIGINAL_BODY_SHA,
               "original rectangle body")

    r = {phase: ram(printer_run, phase) for phase in (
        "first-printer-entry", "first-printer-return",
        "second-printer-entry", "second-printer-return")}
    w = {phase: ram(watch_run, phase) for phase in (
        "row15-clear-entry", "row15-first-store", "row15-clear-return")}
    for phase in ("first-printer-entry", "first-printer-return"):
        if native_route_fields(r[phase]) != ((15, 4, 6), (0, 5, 10)):
            raise ValueError(f"{phase} native route fields changed")
    for phase in ("second-printer-entry", "second-printer-return"):
        if native_route_fields(r[phase]) != ((14, 4, 6), (0, 0, 11)):
            raise ValueError(f"{phase} native route fields changed")
    for phase in ("row15-clear-entry", "row15-clear-return"):
        if native_route_fields(w[phase]) != ((14, 4, 6), (0, 0, 11)):
            raise ValueError(f"{phase} native route fields changed")
    far_call(r["first-printer-entry"][0x08DE0 + 0x2B55:
                                       0x08DE0 + 0x2B5A],
             PRINTER_LOADED_CALL_SHA, (0x0626, 0x0F2D),
             "loaded printer call")
    check_hash(r["first-printer-entry"][0x0F2D0 + 0x0626:
                                          0x0F2D0 + 0x06A7],
               PRINTER_RUNTIME_SHA, "loaded printer body")
    far_call(w["row15-clear-entry"][0x08DE0 + 0x2B42:
                                      0x08DE0 + 0x2B47],
             PARENT_LOADED_CALL_SHA, (0x0137, 0x0C15),
             "loaded parent call")
    far_call(w["row15-clear-entry"][0x0C150 + 0x01F1:
                                      0x0C150 + 0x01F6],
             RECT_LOADED_CALL_SHA, (0x00B7, 0x0F2D),
             "loaded rectangle call")
    check_hash(w["row15-clear-entry"][0x0F2D0 + 0x00B7:
                                      0x0F2D0 + 0x0171],
               RECT_LOADED_BODY_SHA, "loaded rectangle body")
    validate_rect_frame(w["row15-clear-entry"])
    if struct.unpack_from("<6H", w["row15-first-store"],
                          0x1D160 + 0x3B4A) != FIRST_STORE_FRAME:
        raise ValueError("first SYSTEM zero-store frame changed")
    check_hash(w["row15-first-store"][0x131C0 + 0x16B7:
                                        0x131C0 + 0x16B9],
               FIRST_STORE_SHA, "loaded first SYSTEM store instruction")

    p = {phase: page(printer_run, phase) for phase in (
        "first-printer-entry", "first-printer-return",
        "second-printer-entry", "second-printer-return")}
    q = {phase: page(watch_run, phase) for phase in (
        "row15-clear-entry", "row15-clear-return")}
    for before, after in ((p["first-printer-entry"], p["first-printer-return"]),
                          (q["row15-clear-entry"], q["row15-clear-return"]),
                          (p["second-printer-entry"], p["second-printer-return"])):
        validate_delta(before, after)
    if p["first-printer-return"] != q["row15-clear-entry"] or \
            q["row15-clear-return"] != p["second-printer-entry"]:
        raise ValueError("native two-run bridge page boundary changed")

    first = run_probe(
        printer_probe, printer_run / "compose-first-input-private.bin",
        printer_run / "compose-first-output-private.bin",
        printer_input(r["first-printer-entry"], 0x3C6E, SOURCE_SHA[0],
                      p["first-printer-entry"]), (),
        "cells=19 stores=608 changed=210")
    if first != p["first-printer-return"]:
        raise ValueError("first C printer page differs from native return")
    args = ("0", "0", "15", "38", "15", "17", "0")
    live_clear = run_probe(
        rect_probe, watch_run / "compose-live-clear-input-private.bin",
        watch_run / "compose-live-clear-output-private.bin",
        q["row15-clear-entry"], args,
        "stores=704 changed=210 selected=A000 restored=A000")
    if live_clear != q["row15-clear-return"]:
        raise ValueError("C clear differs from immediate native return")
    bridged = run_probe(
        rect_probe, printer_run / "compose-clear-input-private.bin",
        printer_run / "compose-clear-output-private.bin", first, args,
        "stores=704 changed=210 selected=A000 restored=A000")
    if bridged != p["second-printer-entry"]:
        raise ValueError("C bridge differs from second printer entry")
    second = run_probe(
        printer_probe, printer_run / "compose-second-input-private.bin",
        printer_run / "compose-second-output-private.bin",
        printer_input(r["second-printer-entry"], 0x3B7E, SOURCE_SHA[1],
                      bridged), (), "cells=19 stores=608 changed=210")
    if second != p["second-printer-return"]:
        raise ValueError("second C printer page differs from native return")
    print("post-Up route: 08DE:2B55 -> 0C15:01F1 -> 08DE:2B55")
    print("first reset writer: 131C:16B7, ES:DI=A000:12D1, AL=0, CX=22")
    print("rectangle frame: color/page/bottom/right/top/left=0/0/15/38/15/17")
    print("page hashes:", PAGE_SHA["first-printer-entry"],
          PAGE_SHA["first-printer-return"],
          PAGE_SHA["second-printer-entry"],
          PAGE_SHA["second-printer-return"])
    print("three C stages: 32000/32000 bytes each at native boundaries")
    print("net changed values per stage: 210, 210, 210")
    print("native party fields: 15/4/6 -> 14/4/6; window words: 0/5/10 -> 0/0/11")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--printer-run", type=Path, required=True)
    parser.add_argument("--watch-run", type=Path, required=True)
    parser.add_argument("--printer-probe", type=Path, required=True)
    parser.add_argument("--rect-probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.printer_run, args.watch_run,
          args.printer_probe, args.rect_probe)
