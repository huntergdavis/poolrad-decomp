"""Synthetic loaded overlay stubs; no original game bytes are embedded."""
import struct


def physical(seg, off):
    return ((seg << 4) + (off & 65535)) & 0xFFFFF


def install_stub(memory, blob, start, instruction, target_cs, target_ip):
    """Retain the source LCALL slot offset and rebind only its segment word."""
    stub_ip = int(instruction.op_str.split(",")[1].strip(), 16)
    stub_cs = (target_cs + 0x100) & 65535
    offset = instruction.address - start
    blob[offset + 1:offset + 5] = struct.pack("<HH", stub_ip, stub_cs)
    for i, value in enumerate(struct.pack("<BHH", 0xEA, target_ip, target_cs)):
        memory[physical(stub_cs, stub_ip + i)] = value


def loaded_sites(memory, segments, decoded):
    """Actual fixture stub PCs, for executing their EA without body coverage credit."""
    result = set()
    for (kind, ip), ins in decoded.items():
        if ins.mnemonic != "lcall" or kind not in segments:
            continue
        source_seg = int(ins.op_str.split(",")[0].strip(), 16)
        if source_seg not in (0x2B, 0x96, 0xB0, 0x3DB, 0x3E0):
            continue
        cs = segments[kind]
        raw = bytes(memory[physical(cs, ip + i)] for i in range(1, 5))
        off, seg = struct.unpack("<HH", raw)
        if memory[physical(seg, off)] == 0xEA:
            result.add((seg, off))
    return result


def native_stub_patch(instruction, caller_meta, target_meta, target_ip):
    """Assembly using runtime segment words captured in the COM output header."""
    ip = instruction.address
    slot = int(instruction.op_str.split(",")[1].strip(), 16)
    return [f'mov ax,word ptr cs:[output+{caller_meta}]', 'mov es,ax',
            f'mov word ptr es:[{ip+1}],{slot}',
            f'mov ax,word ptr cs:[output+{target_meta}]', 'add ax,0x100',
            f'mov word ptr es:[{ip+3}],ax', 'mov es,ax',
            f'mov byte ptr es:[{slot}],0xea', f'mov word ptr es:[{slot+1}],{target_ip}',
            f'mov ax,word ptr cs:[output+{target_meta}]', f'mov word ptr es:[{slot+3}],ax']
