#!/usr/bin/env python3
"""Independently reconcile a census queue and rank uncredited callable leaf leads.

The queue is a hash-pinned metadata input, not a module dependency. Equal code
hashes are review leads, never evidence for merging different original starts.
Original media stay local; outputs contain addresses, hashes and measured facts.
"""

import argparse
from collections import defaultdict
import csv
import hashlib
import json
from pathlib import Path

from classify_overlay import KNOWN_GAME_OVR_SHA256, KNOWN_START_EXE_SHA256


BUILD = dict(start_exe_sha256=KNOWN_START_EXE_SHA256, game_ovr_sha256=KNOWN_GAME_OVR_SHA256)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def unique(rows):
    result = {}
    for row in rows:
        require(row["id"] not in result, "duplicate input ID")
        result[row["id"]] = row
    return result


def address_registry(inventory, resident):
    """Resolve the inventory/loader coordinate systems without queue aliases."""
    registry, originals, stubs = {}, set(), {}
    def add(alias, identity):
        require(alias not in registry or registry[alias] == identity, "source alias conflict")
        registry[alias] = identity
    for unit in inventory["units"]:
        para = unit["stub_para"]
        starts = {f["rel"] for f in unit["functions"]}
        for rel in starts:
            identity = f"OVL_{para:04X}_{rel:04X}"
            require(identity not in originals, "duplicate inventory original")
            originals.add(identity)
            add(identity, identity)
            add(f"{para:#x}:{rel:#06x}", identity)
        for slot, raw in enumerate(unit["entries"]):
            rel = raw if raw in starts else raw - 0x800
            require(rel in starts, "unresolved entry alias")
            identity = f"OVL_{para:04X}_{rel:04X}"
            add(f"OVL_{para:04X}_{raw:04X}", identity)
            add(f"{para:#x}:{raw:#06x}", identity)
            segment = unit["memory_segment"]
            require(segment is not None, "overlay stub segment missing")
            offset = 0x20 + 5 * slot
            image = 16 * segment + offset
            require(image not in stubs, "overlapping overlay stubs")
            stubs[image] = identity
            add(f"STUB_{segment:04X}_{offset:04X}", identity)
            add(f"RES_IMAGE_{image:05X}", identity)
            add(f"image:0x{image:05X}", identity)
    for row in resident["rows"] + resident["runtime_rows"]:
        image = row["image_offset"]
        identity = stubs.get(image, f"RES_IMAGE_{image:05X}")
        originals.add(identity)
        add(row["id"], identity)
        add(f"RES_IMAGE_{image:05X}", identity)
        add(f"image:0x{image:05X}", identity)
    return registry, originals


def audit_aliases(queue, inventory, resident):
    registry, expected = address_registry(inventory, resident)
    resident_owners = defaultdict(set)
    for source in resident["rows"] + resident["runtime_rows"]:
        identity = registry[source["id"]]
        cls = source["classification"]
        owner = ("game_verified" if cls == "game_verified" else "library_inferred"
                 if cls in ("linked_crt_inferred", "linked_runtime_evidenced") else "unknown")
        resident_owners[identity].add(owner)
    groups, alias_owners = defaultdict(list), {}
    for row in queue["rows"]:
        require(row["id"] in registry, "queue original absent from independent registry")
        identity = registry[row["id"]]
        require(registry.get(row["original_address"]) == identity, "original address disagrees with registry")
        require(row["strict_full_c"] in (0, 1), "invalid strict credit")
        require(not row["strict_full_c"] or row["game_or_library"] == "game_verified",
                "strict credit on unknown or inferred-library ownership")
        if row["id"].startswith("RES_IMAGE_") and identity.startswith("RES_IMAGE_"):
            owners = resident_owners[identity] - {"unknown"}
            require(len(owners) <= 1, "resident source ownership conflict")
            require(row["game_or_library"] == (next(iter(owners)) if owners else "unknown"),
                    "resident ownership differs from source evidence")
        groups[identity].append(row)
        for alias in [row["id"]] + row["aliases"]:
            require(registry.get(alias) == identity, "queue alias resolves to another original")
            require(alias not in alias_owners, "queue alias appears more than once")
            alias_owners[alias] = row["id"]
    # Test credit before the general duplicate-start check, so alias credits
    # cannot silently survive a change in queue row or identifier conventions.
    require(all(sum(r["strict_full_c"] for r in members) <= 1 for members in groups.values()),
            "duplicate strict full-C credit for one logical original")
    require(all(len(members) == 1 for members in groups.values()), "duplicate logical original in queue")
    require(set(groups) == expected, "queue does not cover independently supplied starts")
    return dict(original_starts=len(groups), aliases_checked=len(alias_owners),
                duplicate_logical_originals=0, duplicate_strict_credits=0,
                strict_full_c_originals=sum(r["strict_full_c"] for r in queue["rows"]),
                verified_game_identities=sum(r["game_or_library"] == "game_verified" for r in queue["rows"]),
                unknown_ownership=sum(r["game_or_library"] == "unknown" for r in queue["rows"]),
                whole_game_denominator="unknown")


def walk_leaf(code, start, bound):
    """Confirm a call/service-free reachable CFG inside one candidate bound."""
    from capstone import (Cs, CS_ARCH_X86, CS_MODE_16, CS_GRP_CALL,
                          CS_GRP_INT, CS_GRP_IRET, CS_GRP_JUMP, CS_GRP_RET)
    from capstone.x86 import X86_OP_IMM
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    stop = start + bound
    if start < 0 or bound <= 0 or stop > len(code):
        return None
    pending, decoded, occupied = [start], {}, {}
    returns = {}
    while pending:
        pos = pending.pop()
        if pos in decoded:
            continue
        if not start <= pos < stop or (pos in occupied and occupied[pos] != pos):
            return None
        ins = next(decoder.disasm(code[pos:min(stop, pos + 15)], pos, count=1), None)
        if ins is None or pos + ins.size > stop:
            return None
        if any(b in occupied for b in range(pos, pos + ins.size)):
            return None
        decoded[pos] = ins.size
        occupied.update({b: pos for b in range(pos, pos + ins.size)})
        if ins.group(CS_GRP_CALL) or ins.group(CS_GRP_INT) or ins.group(CS_GRP_IRET):
            return None
        if ins.mnemonic in ("in", "out", "insb", "insw", "outsb", "outsw", "hlt"):
            return None
        if ins.group(CS_GRP_RET):
            returns[pos] = (ins.mnemonic.startswith("retf"),
                            ins.operands[0].imm if ins.operands else 0)
            continue
        if ins.group(CS_GRP_JUMP):
            if (len(ins.operands) != 1 or ins.operands[0].type != X86_OP_IMM
                    or ins.mnemonic == "ljmp"):
                return None
            pending.append(ins.operands[0].imm)
            if ins.mnemonic == "jmp":
                continue
        pending.append(pos + ins.size)
    if not returns:
        return None
    end = max(p + size for p, size in decoded.items())
    return dict(span_bytes=end - start, reachable_bytes=sum(decoded.values()),
                instruction_starts=len(decoded), return_sites=len(returns),
                far_return_sites=sum(far for far, _ in returns.values()),
                near_return_sites=sum(not far for far, _ in returns.values()),
                return_cleanup_bytes=sorted({cleanup for _, cleanup in returns.values()}),
                body_sha256=hashlib.sha256(code[start:end]).hexdigest())


def rank_leaves(queue, features, claims, inventory, overlay):
    feature_map, claim_map = unique(features), unique(claims)
    units = {u["stub_para"]: u for u in inventory["units"]}
    ranked, excluded, hash_groups = [], [], defaultdict(list)
    overlay_rows = [r for r in queue["rows"] if r["domain"] == "overlay"]
    require(set(feature_map) == {r["original_address"] for r in overlay_rows}, "classifier IDs differ from queue")
    for row in overlay_rows:
        feature = feature_map[row["original_address"]]
        require(feature["classification"] == row["classification"], "queue classification differs from source")
        require(row["game_or_library"] == ("game_verified" if feature["classification"] == "game_verified" else "unknown"),
                "overlay ownership differs from source evidence")
        para, rel = (int(s, 16) for s in row["original_address"].split(":"))
        unit = units[para]
        code = overlay[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
        bound = int(feature["bound"])
        require(0 <= rel < rel + bound <= len(code), "candidate bound outside overlay unit")
        digest = hashlib.sha256(code[rel:rel + bound]).hexdigest()
        hash_groups[(bound, digest)].append(row["id"])
        if (row["strict_full_c"] or row["classification"] not in ("game_verified", "game_evidenced")
                or int(feature["calls"]) or not feature["doc_globals"] or row["grade"] not in ("A", "B")):
            continue
        claim = claim_map.get(row["id"])
        if claim is None or claim["status"] != "unclaimed_in_snapshot":
            excluded.append(dict(id=row["id"], reason="claim_unknown" if claim is None else claim["status"]))
            continue
        body = walk_leaf(code, rel, bound)
        if body is None:
            excluded.append(dict(id=row["id"], reason="bounded_leaf_cfg_not_proven"))
            continue
        ranked.append(dict(id=row["id"], grade=row["grade"], classification=row["classification"],
                           game_or_library=row["game_or_library"], strict_full_c=0,
                           claim_status=claim["status"], claim_evidence=claim["evidence"],
                           documented_global_count=len(feature["doc_globals"].split(";")),
                           candidate_bound_bytes=bound, **body))
    ranked.sort(key=lambda r: (r["span_bytes"], r["id"]))
    for rank, row in enumerate(ranked, 1):
        row["rank"] = rank
    duplicates = [dict(bound_bytes=bound, bounded_span_sha256=digest, original_ids=sorted(ids),
                       identity_merge_proven=False)
                  for (bound, digest), ids in sorted(hash_groups.items()) if len(ids) > 1]
    return dict(ranked_leaf_candidates=ranked, excluded_leaf_candidates=sorted(excluded, key=lambda r: r["id"]),
                equal_bounded_byte_leads=duplicates)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ("queue", "queue-sha256", "inventory", "resident", "classification", "claims", "ovr", "json"):
        parser.add_argument("--" + flag, required=True)
    args = parser.parse_args()
    paths = {k: Path(v) for k, v in vars(args).items() if k not in ("queue_sha256", "json")}
    blobs = {k: p.read_bytes() for k, p in paths.items()}
    try:
        require(hashlib.sha256(blobs["queue"]).hexdigest() == args.queue_sha256, "queue artifact hash mismatch")
        require(hashlib.sha256(blobs["ovr"]).hexdigest() == BUILD["game_ovr_sha256"], "owner overlay hash mismatch")
        queue, inventory, resident = (json.loads(blobs[k]) for k in ("queue", "inventory", "resident"))
        for doc in (queue, inventory, resident):
            require(all(doc.get("inputs", {}).get(k) == v for k, v in BUILD.items()), "unsupported input build")
        for key, artifact in (("inventory", "inventory"), ("resident", "resident_audit"),
                              ("classification", "classification")):
            require(hashlib.sha256(blobs[key]).hexdigest() == queue["artifacts_sha256"][artifact],
                    "audit input differs from queue provenance")
        summary = audit_aliases(queue, inventory, resident)
        require(summary["strict_full_c_originals"] == 66 and summary["verified_game_identities"] == 128,
                "reviewed strict-C or verified-identity checkpoint differs")
        result = dict(inputs=BUILD, alias_audit=summary,
                      artifacts_sha256={k: hashlib.sha256(v).hexdigest() for k, v in sorted(blobs.items())},
                      **rank_leaves(queue, list(csv.DictReader(blobs["classification"].decode().splitlines())),
                                    list(csv.DictReader(blobs["claims"].decode().splitlines())), inventory, blobs["ovr"]))
    except (ValueError, KeyError, TypeError) as error:
        parser.exit(2, f"alias audit refused: {error}\n")
    Path(args.json).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps(dict(summary=summary, ranked_leaf_candidates=len(result["ranked_leaf_candidates"]),
                          equal_bounded_byte_groups=len(result["equal_bounded_byte_leads"])), sort_keys=True))


if __name__ == "__main__":
    main()
