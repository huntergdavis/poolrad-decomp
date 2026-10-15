#!/usr/bin/env python3
"""One-instruction source/loaded-image identity audit; no guest execution."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ENTRY, WIDTH, SOURCE_SHA = 0x0905, 1, "cfae0d4248f7142f7b17f826cd7a519280e312577690e957830d23dcf35a3fff"

def verify_loaded_image(image, cs, entry, code):
    if len(image) != 1048576:
        raise ValueError("loaded image must contain exactly 1 MiB")
    if any(type(value) is not int or not 0 <= value <= 65535 for value in (cs, entry)):
        raise ValueError("loaded CS and entry must be supplied 16-bit integers")
    if not 1 <= len(code) <= 15 or entry + len(code) > 65536:
        raise ValueError("instruction length/offset crosses approved loaded domain")
    addresses = [(cs * 16 + entry + i) % 1048576 for i in range(len(code))]
    actual = bytes(image[address] for address in addresses)
    if actual != code:
        raise ValueError("loaded instruction bytes differ from original source")
    return {"loaded_cs": cs, "loaded_ip": entry, "byte_addresses": addresses,
            "loaded_instruction_bytes_match_source": True,
            "loaded_instruction_sha256": sha256(actual).hexdigest(),
            "image_origin": "caller supplied; provenance not inferred",
            "guest_execution_or_frame_join": False}

def required_stack_addresses(ss, sp):
    """Unexecuted first-word input requirements shared by prior frame candidates."""
    if any(type(v) is not int or not 0 <= v <= 65535 for v in (ss, sp)):
        raise ValueError("SS and SP must be supplied 16-bit integers")
    first = (ss * 16 + sp) % 1048576
    addresses = [first, (first + 1) % 1048576]
    return {
        "supplied_SS": ss, "supplied_SP": sp,
        "input_word_straddles_offset_boundary": sp == 65535,
        "candidates": {
            "wrapped_offset_word_starts": {"input_word_byte_addresses": list(addresses)},
            "contiguous_physical_frame": {"input_word_byte_addresses": list(addresses)}},
        "candidate_addresses_differ": False,
        "word_byte_candidate_convention": "physical adjacent bytes; A20 off; unexecuted requirement",
        "selected_policy": None, "native_wrap_policy_resolved": False,
        "stack_contents_read": False, "pop_value_read": False,
        "cpu_transition_evaluated": False,
        "evidence_status": "single-word input requirement only; contents and runtime effects unknown",
    }

def audit(root, loaded_image=None, loaded_cs=None, ss=None, sp=None):
    if (loaded_image is None) != (loaded_cs is None):
        raise ValueError("supply loaded image and CS together")
    if (ss is None) != (sp is None):
        raise ValueError("supply SS and SP together")
    addresses = None if ss is None else required_stack_addresses(ss, sp)
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    media = (root / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(media) != 206105 or sha256(media).hexdigest() != "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614":
        raise ValueError("media provenance differs")
    if sha256(media[196813:197723]).hexdigest() != "59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae":
        raise ValueError("assigned entry4 differs")
    start = 195410 + ENTRY
    code = media[start:start + WIDTH]
    if len(code) != WIDTH or sha256(code).hexdigest() != SOURCE_SHA:
        raise ValueError("bounded source identity differs")
    decoded = [(i.address, i.size, i.mnemonic, i.op_str)
               for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, ENTRY)]
    if decoded != [(0x0905, 1, 'pop', 'bp')]:
        raise ValueError("bounded source shape differs")
    loaded = None if loaded_image is None else verify_loaded_image(loaded_image.read_bytes(), loaded_cs, ENTRY, code)
    return {"target": "U3C2 +0905", "file_extent": [start, start + WIDTH],
            "offset_extent": [ENTRY, ENTRY + WIDTH], "source_sha256": SOURCE_SHA,
            "instructions": 1, "address_requirements": addresses,
            "stack_contents_read": False, "pop_value_read": False,
            "cpu_transition_evaluated": False, "selected_policy": None,
            "native_wrap_policy_resolved": False, "source_instruction_end": 0x0906, "loaded_image": loaded,
            "ram_image_read_by_audit": loaded_image is not None,
            "upstream_native_AF_known": False, "execution_fixture_implemented": False,
            "guest_instruction_execution": False, "original_frame_join": False,
            "producer_consistency_inferred": False, "native_credit": 0,
            "strict_full_c_credit": 0, "retf_policy_selected": False}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--loaded-image", type=Path)
    parser.add_argument("--loaded-cs", type=lambda value: int(value, 0))
    parser.add_argument("--ss", type=lambda value: int(value, 0))
    parser.add_argument("--sp", type=lambda value: int(value, 0))
    args = parser.parse_args()
    if (args.ss is None) != (args.sp is None):
        parser.error("supply --ss and --sp together")
    if (args.loaded_image is None) != (args.loaded_cs is None):
        parser.error("supply --loaded-image and --loaded-cs together")
    try:
        report = audit(args.root, args.loaded_image, args.loaded_cs, args.ss, args.sp)
    except (ImportError, OSError, ValueError) as error:
        parser.exit(1, f"static audit failed: {error}\n")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps(report, sort_keys=True))

if __name__ == "__main__":
    main()
