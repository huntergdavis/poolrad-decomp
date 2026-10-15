#!/usr/bin/env python3
"""Reset CPU fragments; original DOS effects remain an explicit gap."""
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
PREFIX=['031f', '1410', '1413', '1415', '1417', '141f', '1420', '1422', '1425', '142b', '1447', '144c', '144e', '144f', '1452', '1453', '1454', '1456']
SUFFIX=['1458', '1459', '1461', '1467', '146a', '146d', '1471', '1472', '0324']
RAW_HASH='389b432f09fdaaadf1430e48ae5f340e1bb85a0e84b606577ed38f143bf9a02c'
BODY_HASH='18b5680c7d5ea7bfdee3902b7a11279f62216c07fbff9c692fb8820363b481f0'
RAM_HASHES={'031f': '45e47818a8132017388a2e1fc98a03b076adba8eee978f824cd26ba1e01b02fc', '1410': 'ec6d6dda441cfae339f4fc064b45ae0555d87bd4c603fb93ac962f42165211db', '1413': 'ec6d6dda441cfae339f4fc064b45ae0555d87bd4c603fb93ac962f42165211db', '1415': 'ec6d6dda441cfae339f4fc064b45ae0555d87bd4c603fb93ac962f42165211db', '1417': 'ec6d6dda441cfae339f4fc064b45ae0555d87bd4c603fb93ac962f42165211db', '141f': 'ec6d6dda441cfae339f4fc064b45ae0555d87bd4c603fb93ac962f42165211db', '1420': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '1422': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '1425': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '142b': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '1447': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '144c': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '144e': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '144f': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '1452': 'aaa7cb2cbfda1beb6d6970f696e7d1f7f31b20b4e3ce722ec93ded188680edb7', '1453': '6586ef8fe4aec9351df990af306c33514eab86c0d3952f90db1cbf34dc551d9e', '1454': '6586ef8fe4aec9351df990af306c33514eab86c0d3952f90db1cbf34dc551d9e', '1456': '6586ef8fe4aec9351df990af306c33514eab86c0d3952f90db1cbf34dc551d9e', '1458': '9b2b5e70436540e2c722f06c87227ffca08bfcedb2c90092298623bcc5419501', '1459': '9b2b5e70436540e2c722f06c87227ffca08bfcedb2c90092298623bcc5419501', '1461': '9b2b5e70436540e2c722f06c87227ffca08bfcedb2c90092298623bcc5419501', '1467': '81fb41f02e5e085078d48adf7b9aecaadb733592374051a69ca03be8253121e3', '146a': '009e6801b63951b47f1b306f04b633721f0a8a2c0b549c2f5471e06b6b255a0e', '146d': '009e6801b63951b47f1b306f04b633721f0a8a2c0b549c2f5471e06b6b255a0e', '1471': '9273f5159bcfa20ade283aea55af48634836e8adb7cb0cde94d74be6fb2be3b9', '1472': '9273f5159bcfa20ade283aea55af48634836e8adb7cb0cde94d74be6fb2be3b9', '0324': '9273f5159bcfa20ade283aea55af48634836e8adb7cb0cde94d74be6fb2be3b9'}
class Report(ct.Structure):
    _fields_=[('valid',ct.c_uint)]+[(n,ct.c_uint16) for n in 'original_segment call_pc callee_pc return_pc ax ds dx'.split()]

def inputs(root):
    raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
    clean=re.sub(chr(27)+r'\[[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
    rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
    names=PREFIX+SUFFIX
    assert len(rows)==len(names)+1 and rows[0]==rows[1]  # retry after missing python alias
    rows=rows[1:]
    cpus={n:[int(v,16) for v in row.split()] for n,row in zip(names,rows)}
    images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,RAM_HASHES[n]) for n in names}
    start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
    mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
    mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
    body=start[0xC086:0xC0EB];assert len(body)==101 and sha256(body).hexdigest()==BODY_HASH
    call=start[0x8057+0x31F:0x8057+0x324]
    assert call==bytes.fromhex('9a1014f80a')
    loaded=call[:3]+(0x131C).to_bytes(2,'little')
    for n,values in cpus.items():
        assert len(values)==16 and values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
        assert values[:2]==[0x1026 if n in ('031f','0324') else 0x131C,int(n,16)]
        image=images[n]
        assert image[0x131C0+0x1410:0x131C0+0x1475]==body
        assert image[0x10260+0x31F:0x10260+0x324]==loaded
    assert cpus['1456'][9]==0x3D02 and cpus['1456'][2]==0x1D16 and cpus['1456'][12]==0x3300
    assert cpus['1458'][9]==6 and not cpus['1458'][-1]&1
    changes=[i for i,(a,b) in enumerate(zip(images['1456'],images['1458'])) if a!=b]
    assert len(changes)==18
    assert images['1456'][0x20430:0x204B0]==images['1458'][0x20430:0x204B0]
    before,after=(images[n][0x20430:0x204B0] for n in ('1458','0324'))
    assert before[:6]==bytes.fromhex('0000b0d70000')
    assert after[:6]==bytes.fromhex('0600b3d78000') and before[6:]==after[6:]
    assert cpus['0324'][7:9]==[0x32D0,0x337A]
    return cpus,images

def differential(lib,label,image,values,terminal,status,checkpoints=()):
    memory=ct.create_string_buffer(image,len(image))
    state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
    uc=mapped.emulator(image,values);pending=list(checkpoints);seen=0;started=time.monotonic()
    for step in range(100):
        pc=(state.cpu.cs,state.cpu.ip)
        if time.monotonic()-started>=10:raise TimeoutError(f'{label}: deadline at {pc}')
        before_cpu,before_ram=cpu_values(state.cpu),memory.raw
        report=Report();result=lib.pr_system_reset_1410_step(ct.byref(state),ct.byref(report))
        if result:
            assert pc==terminal and result==status and not pending,(label,pc,result)
            assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
            if status==1:
                assert (report.valid,report.original_segment,report.call_pc,report.callee_pc,report.return_pc)==(1,0xAF8,0x1456,0,0x1458)
                assert (report.ax,report.ds,report.dx)==(state.cpu.ax,state.cpu.ds,state.cpu.dx)
            elif status==2:
                assert (report.valid,report.original_segment,report.call_pc,report.callee_pc,report.return_pc)==(1,0xAF8,0x1442,0x1491,0x1445)
            else:assert not report.valid
            print(f'{label}: {step} full-state comparisons; {seen} native checkpoints; typed stop {status}',flush=True)
            return step,seen
        assert not report.valid
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
        assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
        if pending:
            n,expected_cpu,expected_ram=pending.pop(0)
            compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;seen+=1
    raise AssertionError(f'{label}: instruction bound')

def main():
    if len(sys.argv)!=3:raise SystemExit('usage: audit_system_reset_1410_abi.py LIBRARY NATIVE_ROOT')
    if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
    library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
        raise SystemExit('library and inputs must stay in this clone resources/')
    cpus,images=inputs(root);lib=ct.CDLL(str(library))
    lib.pr_system_reset_1410_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)]
    lib.pr_system_reset_1410_step.restype=ct.c_int
    total=native=0
    for label,plan,terminal,status in [('native-before-DOS',PREFIX,(0x131C,0x1456),1),('native-after-DOS-independent',SUFFIX,(0x1026,0x324),3)]:
        first=plan[0]
        count,seen=differential(lib,label,images[first],cpus[first],terminal,status,[(n,cpus[n],images[n]) for n in plan[1:]])
        total+=count;native+=seen
    # Every controlled case starts with its own copied input. None crosses INT21.
    for signature in (0,0xD7B1,0xD7B3):
        image=bytearray(images['031f']);image[0x20432:0x20434]=signature.to_bytes(2,'little')
        term,status=((0x131C,0x1442),2) if signature==0xD7B3 else ((0x1026,0x324),3)
        count,_=differential(lib,f'controlled-signature-{signature:04x}',bytes(image),cpus['031f'],term,status);total+=count
    for mode in (0,1,2,255):
        for flags in (0x7202,0x7602,0x7AD7):
            image=bytearray(images['031f']);image[0x14A00+0x442C]=mode
            values=cpus['031f'].copy();values[-1]=flags
            count,_=differential(lib,f'controlled-mode-{mode}-flags-{flags:04x}',bytes(image),values,(0x131C,0x1456),1);total+=count
    image=bytearray(images['031f']);image[0x20460]=0
    count,_=differential(lib,'controlled-empty-name-no-DOS',bytes(image),cpus['031f'],(0x1026,0x324),3);total+=count
    # Isolated error-tail CPU tests, not native DOS errors or callback results.
    for ax in (2,5,0xFFFF):
        values=cpus['1458'].copy();values[9]=ax;values[-1]|=1
        count,_=differential(lib,f'controlled-independent-error-tail-{ax}',images['1458'],values,(0x1026,0x324),3);total+=count
    image=bytearray(images['1472']);values=cpus['1472'].copy();values[7]=0xFFFC
    image[0x1D160+0xFFFC:0x1D160+0x10000]=bytes.fromhex('24032610')
    count,_=differential(lib,'controlled-RETF6-stack-wrap',bytes(image),values,(0x1026,0x324),3);total+=count
    print(f'PASS {total} full-state comparisons; {native} native post-input checkpoints; 27 native states / 28 EV rows; DOS 18-byte RAM delta excluded from C',flush=True)
if __name__=='__main__':main()
