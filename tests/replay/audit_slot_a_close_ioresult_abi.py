#!/usr/bin/env python3
"""Original Close/IOResult CPU parity with an explicit DOS gap."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import time
import unicorn
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/replay"))
import audit_slot_a_delete_abi_map as mapped
from audit_slot_a_string_0356_abi import CPU,State,NAMES,cpu_values,compare_cpu
PREFIX=['0324', '0328', '0329', '032a', '1491', '1493', '1497', '14b6', '14bc', '14c4', '149a', '149c', '149f', '14a2', '14a4', '14a6']
SUFFIX=['14a8', '14ad', '14b3', '032f', '04a2', '04a4', '04a8', '0334', '0337', '033c', '033e', '0342', '0348', '034a', '034d', '0350', '0352', '0353', '0292']
RAW_HASH='cfaf06b175e54aeb3e2fbc3f0f3099baac3a409059f20aa0c3c1dfaa47c545d6'
RAM_HASHES={'0324': 'bed4631c0e9ab0de20adcc222906e2cce68eb846b1e47623f7b1bf11844b7e02', '0328': 'bed4631c0e9ab0de20adcc222906e2cce68eb846b1e47623f7b1bf11844b7e02', '0329': 'bed4631c0e9ab0de20adcc222906e2cce68eb846b1e47623f7b1bf11844b7e02', '032a': 'bed4631c0e9ab0de20adcc222906e2cce68eb846b1e47623f7b1bf11844b7e02', '1491': 'e1f9fb1893af563f5f2353b20c172829dba242f441674e48a826812c0c442731', '1493': 'e1f9fb1893af563f5f2353b20c172829dba242f441674e48a826812c0c442731', '1497': 'e1f9fb1893af563f5f2353b20c172829dba242f441674e48a826812c0c442731', '14b6': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '14bc': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '14c4': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '149a': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '149c': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '149f': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '14a2': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '14a4': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '14a6': 'd455dee3e801fe774f0fdfe3d0f301f024e32d7838b577bb3564b9df1d5e8c31', '14a8': 'e6000584c33803a491449a91a220d3af19c0fbee21a2bf571cc934e50aafba0c', '14ad': 'e6000584c33803a491449a91a220d3af19c0fbee21a2bf571cc934e50aafba0c', '14b3': 'e3fd504e275291f3637e09aab09589ec33b5f125591c04b4ee32290ffa59ef6f', '032f': 'e3fd504e275291f3637e09aab09589ec33b5f125591c04b4ee32290ffa59ef6f', '04a2': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '04a4': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '04a8': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '0334': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '0337': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '033c': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '033e': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '0342': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '0348': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '034a': 'd39bfb845f431a603de64d50a6373a762e78db0de1a5a9e87933d4a4a1c02a24', '034d': 'dff07482fe309926993c92629366f6f990dd5b66768ccdae54849e7432207ef1', '0350': 'dff07482fe309926993c92629366f6f990dd5b66768ccdae54849e7432207ef1', '0352': 'dff07482fe309926993c92629366f6f990dd5b66768ccdae54849e7432207ef1', '0353': 'dff07482fe309926993c92629366f6f990dd5b66768ccdae54849e7432207ef1', '0292': 'dff07482fe309926993c92629366f6f990dd5b66768ccdae54849e7432207ef1'}
class Report(ct.Structure):
    _fields_=[('valid',ct.c_uint)]+[(n,ct.c_uint16) for n in 'original_segment call_pc return_pc ax bx'.split()]

def inputs(root):
    raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
    clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
    rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
    names=PREFIX+SUFFIX;assert len(rows)==len(names)==35
    cpus={n:[int(v,16) for v in row.split()] for n,row in zip(names,rows)}
    images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,RAM_HASHES[n]) for n in names}
    start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
    mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
    mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
    bodies=[]
    for pc,n,digest in [(0x1491,52,'83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858'),(0x4A2,7,'d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b')]:
        body=start[0xAC76+pc:0xAC76+pc+n];assert sha256(body).hexdigest()==digest
        bodies.append((pc,body))
    tail=start[0x8057+0x324:0x8057+0x356]
    assert sha256(tail).hexdigest()=='ec66eb474eafa862b715fe8ddbc47161fea211fe275d0807ffe7e7e9ea876208'
    loaded=bytearray(tail)
    for pc,callee in ((0x32A,0x1491),(0x32F,0x4A2)):
        i=pc-0x324;assert loaded[i:i+5]==bytes([0x9A])+callee.to_bytes(2,'little')+(0xAF8).to_bytes(2,'little')
        loaded[i+3:i+5]=(0x131C).to_bytes(2,'little')
    for n,values in cpus.items():
        assert len(values)==16 and values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
        assert values[:2]==[0x131C if int(n,16)>=0x4A2 else 0x1026,int(n,16)]
        image=images[n]
        for pc,body in bodies:assert image[0x131C0+pc:0x131C0+pc+len(body)]==body
        assert image[0x10260+0x324:0x10260+0x356]==loaded
    def words(label,off,n):
        image=images[label];return [int.from_bytes(image[0x1D160+off+2*i:0x1D160+off+2*i+2],'little') for i in range(n)]
    assert words('032a',0x32CC,2)==[0x32D0,0x1D16]
    assert words('1491',0x32C8,4)==[0x32F,0x1026,0x32D0,0x1D16]
    assert words('14b6',0x32C6,5)==[0x149A,0x32F,0x1026,0x32D0,0x1D16]
    assert words('04a2',0x32CC,2)==[0x334,0x1026]
    assert cpus['14a6'][9:11]==[0x3E80,6] and cpus['14a8'][9:11]==[0x3E01,6]
    assert not cpus['14a8'][-1]&1
    assert all(cpus['14a6'][i]==cpus['14a8'][i] for i in range(16) if i not in (1,9))
    assert images['0324'][0x204B0]==23
    assert images['04a2'][0x18E26:0x18E28]==bytes(2)
    assert sum(a!=b for a,b in zip(images['14a6'],images['14a8']))==25
    assert images['14a6'][0x20430:0x204B0]==images['14a8'][0x20430:0x204B0]
    a,b=(images[n][0x20430:0x204B0] for n in ('14a8','032f'))
    assert a[:6]==bytes.fromhex('0600b3d78000') and b[:6]==bytes.fromhex('0600b0d78000')
    assert a[:2]==b[:2] and a[4:]==b[4:]
    assert images['04a2']==images['0334']  # native zero IOResult; frame already pushed
    assert cpus['0292'][7:11]==[0x3384,0x33D8,1,6] and cpus['0292'][-1]==0x7206
    return cpus,images

def differential(lib,label,image,values,terminal,status,checkpoints=()):
    memory=ct.create_string_buffer(image,len(image))
    state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
    uc=mapped.emulator(image,values);pending=list(checkpoints);seen=own=0;started=time.monotonic()
    for step in range(100):
        pc=(state.cpu.cs,state.cpu.ip)
        if time.monotonic()-started>=10:raise TimeoutError(f'{label}: deadline at {pc}')
        before_cpu,before_ram=cpu_values(state.cpu),memory.raw
        report=Report();result=lib.pr_slot_a_close_ioresult_step(ct.byref(state),ct.byref(report))
        if result:
            assert pc==terminal and result==status and not pending,(label,pc,result)
            assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
            if status==1:
                assert (report.valid,report.original_segment,report.call_pc,report.return_pc)==(1,0xAF8,0x14A6,0x14A8)
                assert (report.ax,report.bx)==(state.cpu.ax,state.cpu.bx)
            else:assert not report.valid
            print(f'{label}: {step} full-state comparisons ({own} new); {seen} native checkpoints; typed stop {status}',flush=True)
            return step,own,seen
        assert not report.valid;mapped.one(uc)
        compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
        assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
        own+=pc[0]==0x131C or pc[1] in (0x32A,0x32F)
        if pending:
            n,expected_cpu,expected_ram=pending.pop(0)
            compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;seen+=1
    raise AssertionError(f'{label}: instruction bound')
def main():
    if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_close_ioresult_abi.py LIBRARY NATIVE_ROOT')
    if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
    library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
        raise SystemExit('library and inputs must stay in this clone resources/')
    cpus,images=inputs(root);lib=ct.CDLL(str(library))
    lib.pr_slot_a_close_ioresult_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)]
    lib.pr_slot_a_close_ioresult_step.restype=ct.c_int
    total=own=native=0
    def run(label,image,values,terminal=(0x1026,0x292),status=2,checkpoints=()):
        nonlocal total,own,native
        n,o,v=differential(lib,label,image,values,terminal,status,checkpoints)
        total+=n;own+=o;native+=v
    for label,plan,terminal,status in [('native-before-Close-DOS',PREFIX,(0x131C,0x14A6),1),('native-after-DOS-independent',SUFFIX,(0x1026,0x292),2)]:
        first=plan[0];run(label,images[first],cpus[first],terminal,status,[(n,cpus[n],images[n]) for n in plan[1:]])
    # Initial copies only; no DOS instruction executes or result replaces a live state.
    for signature in (0,0xD7B0,0xFFFF):
        image=bytearray(images['0324']);image[0x20432:0x20434]=signature.to_bytes(2,'little')
        run(f'controlled-invalid-signature-{signature:04x}',bytes(image),cpus['0324'])
    for handle in range(5):
        image=bytearray(images['0324']);image[0x20430:0x20432]=handle.to_bytes(2,'little')
        run(f'controlled-reserved-handle-{handle}-no-DOS',bytes(image),cpus['0324'])
    for handle in (5,7,0x8000,0xFFFF):
        image=bytearray(images['0324']);image[0x20430:0x20432]=handle.to_bytes(2,'little')
        run(f'controlled-handle-{handle}-DOS-gate',bytes(image),cpus['0324'],(0x131C,0x14A6),1)
    for flags in (0x7202,0x7602,0x7AD7):
        image=bytearray(images['0324']);image[0x20430:0x20432]=(4).to_bytes(2,'little')
        values=cpus['0324'].copy();values[-1]=flags
        run(f'controlled-flags-{flags:04x}-no-DOS',bytes(image),values)
    for error in (0,1,0x100,0x8000,0xFFFF):
        for length in (0,23):
            image=bytearray(images['032f']);image[0x14A00+0x4426:0x14A00+0x4428]=error.to_bytes(2,'little')
            image[0x1D160+0x3350]=length
            run(f'controlled-independent-IOResult-{error:04x}-length-{length}',bytes(image),cpus['032f'])
    # Synthetic post-DOS error-tail CPU input, not a measured DOS error contract.
    for error in (5,0x100,0xFFFF):
        values=cpus['14a8'].copy();values[9]=error;values[-1]|=1
        run(f'controlled-independent-Close-error-tail-{error:04x}',images['14a8'],values)
    for first in ('14b3','04a8'):
        image=bytearray(images[first]);values=cpus[first].copy();values[7]=0xFFFC
        target=0x32F if first=='14b3' else 0x334
        image[0x1D160+0xFFFC:0x1D160+0x10000]=target.to_bytes(2,'little')+(0x1026).to_bytes(2,'little')
        run(f'controlled-{first}-stack-wrap',bytes(image),values)
    # Guest alias: XCHG clears its own return IP; original RETF then reaches unsupported0000.
    image=bytearray(images['04a4']);values=cpus['04a4'].copy();values[2]=0x1C00;values[7]=0x32C6
    image[0x20426:0x2042A]=bytes.fromhex('34032610')
    run('controlled-XCHG-return-frame-alias',bytes(image),values,(0x1026,0),4)
    assert native==33
    print(f'PASS {total} full-state comparisons ({own} new, {total-own} unchanged helper); {native} native checkpoints;35 native states; DOS25-byte RAM delta excluded from C',flush=True)
if __name__=='__main__':main()
