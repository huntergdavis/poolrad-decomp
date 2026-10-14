#!/usr/bin/env python3
"""Original-byte/native full-state parity for SYSTEM B98 and saved callers."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import unicorn
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/replay'))
from audit_slot_a_string_0356_abi import CPU,State,Report,NAMES,REGS,cpu_values,compare_cpu
import audit_slot_a_delete_abi_map as mapped
RAM_SIZE=655360

RAM_HASHES = {
    "1999": "6cc4831fcbb883b06fb20e7082b0452f50e7c7adb96cf4fffdda4020eecb1f01",
    "0b98-first": "7aad54745b94d2dbae67db1c4392d15202c4c3f1e244f64cd93853d9eab10f5b",
    "0bab-first": "7aad54745b94d2dbae67db1c4392d15202c4c3f1e244f64cd93853d9eab10f5b",
    "0bae-first": "83f97479a8ce9ee73e5f3bbcf8c93e98983f987f8405e95502303057f8820630",
    "0bbd-first": "83f97479a8ce9ee73e5f3bbcf8c93e98983f987f8405e95502303057f8820630",
    "0bbd-first-partial": "5b4098bb578073fd60096f2718ede0959bcccfbc0569e909b1a76f5a4db7ca28",
    "0bbf-first": "525e143dab0072dbf7e9bf501af465f43b603232c234c3a45ef0e37f50eb0fc0",
    "0bc1-first": "525e143dab0072dbf7e9bf501af465f43b603232c234c3a45ef0e37f50eb0fc0",
    "199e": "525e143dab0072dbf7e9bf501af465f43b603232c234c3a45ef0e37f50eb0fc0",
    "19a3": "be694c2624aa209d769d5feb0a0281ccb189bcb7475ca6b41f7fb4e91103d016",
    "0b98-second": "9f7d905e7961f424aa0f245bd44d62371ecf89afbb7e008883e1430e4c258acc",
    "0bab-second": "9f7d905e7961f424aa0f245bd44d62371ecf89afbb7e008883e1430e4c258acc",
    "0bae-second": "0fba092a6bdb885173e3e49ad25b48ae758d9873bed75a2fac0320379bb94a48",
    "0bbf-second": "097e65f50e4c2066db2353e291b0ea2b751d59a001fc2b60ad4537cb23b904ec",
    "0bc1-second": "097e65f50e4c2066db2353e291b0ea2b751d59a001fc2b60ad4537cb23b904ec",
    "19a8": "097e65f50e4c2066db2353e291b0ea2b751d59a001fc2b60ad4537cb23b904ec"
}

def natural(root):
    raw=(root/'native.log').read_bytes()
    assert sha256(raw).hexdigest()=='04ae0e9fd46294b29e20d6c4cae16bb19d33d99e65dc2c884599301db496db12'
    text=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
    rows=re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)",text)
    assert len(rows)==len(RAM_HASHES)==16
    cpus={name:[int(v,16) for v in row.split()] for name,row in zip(RAM_HASHES,rows)}
    images={name:mapped.pins.checked(root/('ram-'+name+'.bin'),RAM_SIZE,digest) for name,digest in RAM_HASHES.items()}
    for name,v in cpus.items():
        assert len(v)==16 and v[:2]==[0x131C if name.startswith('0b') else 0x2A8E,int(name.split('-')[0],16)]
        assert v==[json.loads((root/('cpu-'+name+'.json')).read_text())[n] for n in NAMES]
    start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
    game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
    mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
    body=start[0xB80E:0xB83A]
    assert sha256(body).hexdigest()=='c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64'
    caller=game[0x1DFE6+0x1999:0x1DFE6+0x19AD]
    assert sha256(caller).hexdigest()=='91bd3d3e5018ab2f3ee5b3a2cb0c32eb12a75e87b2ed8b186aa5915015907407'
    loaded=bytearray(caller)
    for pc,target in ((0x1999,0x131C),(0x19A3,0x131C),(0x19A8,0x1026)):
        i=pc-0x1999
        assert caller[i]==0x9A
        loaded[i+3:i+5]=target.to_bytes(2,'little')
    assert sha256(loaded).hexdigest()=='1b8536d59827bf0c2a130552afc944077464d9c88058af2a16c95530702c9289'
    for ram in images.values():
        assert ram[0x131C0+0xB98:0x131C0+0xBC4]==body
        assert ram[0x2A8E0+0x1999:0x2A8E0+0x19AD]==loaded
    changed=lambda a,b:{i for i,(x,y) in enumerate(zip(images[a],images[b])) if x!=y}
    assert changed('0bab-first','0bae-first')=={0x20C18}
    assert changed('0bbd-first','0bbd-first-partial')=={0x20C24}
    assert changed('0bbd-first','0bbf-first')==set(range(0x20C24,0x20C2C))
    assert changed('0bab-second','0bae-second')=={0x20C18}
    assert len(changed('1999','19a8'))==16
    return cpus,images


def differential(lib,label,image,values,terminal,route=False,checkpoints=None,partial=None):
    memory=ct.create_string_buffer(image,len(image))
    state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
    report=Report();uc=mapped.emulator(image,values)
    fn=lib.pr_slot_a_concat_1999_step if route else lib.pr_system_b98_step
    pending=list(checkpoints or []);seen=0;partial_seen=False
    for steps in range(1000):
        pc=(state.cpu.cs,state.cpu.ip)
        before_cpu,before_ram=cpu_values(state.cpu),memory.raw
        status=fn(ct.byref(state),ct.byref(report)) if route else fn(ct.byref(state))
        if pc==terminal:
            assert status==(1 if route else 3)
            assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
            assert not pending and (not partial or partial_seen)
            if route:
                assert (report.dependency_valid,report.caller,report.call_pc,report.callee_segment,
                        report.callee_pc,report.return_pc,report.caller_is_overlay)==(1,0x6F,0x19A8,0x802,0x2EB,0x19AD,1)
            print(f'{label}: {steps} C instructions, full CPU/RAM original-byte parity, {seen} native C checkpoints'+(', one native REP iteration checked separately' if partial else ''))
            return steps
        assert status==0,(label,pc,status)
        for _ in range(65537):
            uc.emu_start((uc.reg_read(REGS[0])<<4)+uc.reg_read(REGS[1]),0x100000,count=1)
            current=mapped.values(uc)
            if partial and not partial_seen and current==partial[0]:
                assert bytes(uc.mem_read(0,len(image)))==partial[1]
                partial_seen=True
            if tuple(current[:2])!=pc:break
        else:raise AssertionError('original instruction did not finish')
        compare_cpu(cpu_values(state.cpu),mapped.values(uc),label+f' after {pc}')
        assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
        if pending and cpu_values(state.cpu)[:2]==pending[0][1][:2]:
            name,expected_cpu,expected_ram=pending.pop(0)
            compare_cpu(cpu_values(state.cpu),expected_cpu,name)
            assert memory.raw==expected_ram,name
            seen+=1
    raise AssertionError('B98 instruction bound exhausted')


def main():
    if len(sys.argv)!=4:raise SystemExit('usage: audit_system_b98_abi.py LIBRARY NATURAL_B98_ROOT DELETE_MAP_ROOT')
    if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library,root,delete_root=(Path(arg).resolve(strict=True) for arg in sys.argv[1:])
    if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root,delete_root)):
        raise SystemExit('library and native inputs must stay in this clone resources/')
    lib=ct.CDLL(str(library))
    lib.pr_system_b98_step.argtypes=[ct.POINTER(State)];lib.pr_system_b98_step.restype=ct.c_int
    lib.pr_slot_a_concat_1999_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)]
    lib.pr_slot_a_concat_1999_step.restype=ct.c_int
    cpus,images=natural(root)
    checkpoints=[(n,cpus[n],images[n]) for n in RAM_HASHES if n not in ('1999','0bbd-first-partial')]
    differential(lib,'natural saved-A two concatenations',images['1999'],cpus['1999'],(0x2A8E,0x19A8),True,
                 checkpoints,(cpus['0bbd-first-partial'],images['0bbd-first-partial']))
    dcpu,dimages=mapped.inputs(delete_root)
    mapped.code(delete_root,dimages['0b98'])
    differential(lib,'measured nested Delete B98',dimages['0b98'],dcpu['0b98'],(0x131C,0xCFF),
                 checkpoints=[('native-Delete-return',dcpu['0cff'],dimages['0cff'])])
    for name,dlen,slen,alias in (
        ('empty',0,0,''),('exact-capacity',200,55,''),('overflow',200,56,''),
        ('full-destination',255,255,''),('self-alias',3,3,'self'),
        ('forward-overlap',3,5,'forward'),('backward-overlap',3,8,'backward'),
        ('cross-segment-alias',3,3,'segment'),('zero-pointer-wrap',0,0,'wrap'),
        ('add-pointer-carry',3,1,'carry'),
    ):
        image=bytearray(images['0b98-first']);v=cpus['0b98-first'].copy()
        v[7]=0x8000;v[8]=0x8100;v[9]=0xA512;v[11]=0xB601;v[15]|=0x400
        ss=v[6];src_seg=dst_seg=ss;src=0x5000;dst=0x5200
        if alias in ('self','segment'):dst=src
        if alias=='segment':dst_seg+=1;dst-=16
        if alias=='forward':dst=0x5000;src=dst+1
        if alias=='backward':dst=0x5000;src=dst+10
        if alias=='wrap':dst=0xFFFF
        if alias=='carry':dst=0xFFFE
        base=dst_seg<<4
        image[base+dst]=dlen
        if alias not in ('wrap','carry'):
            image[base+dst+1:base+dst+1+dlen]=b'Z'*dlen
        base=src_seg<<4
        image[base+src:base+src+1+slen]=bytes([slen])+bytes(65+i%26 for i in range(slen))
        frame=[0xCFF,0x131C,src,src_seg,dst,dst_seg]
        for i,value in enumerate(frame):
            offset=(ss<<4)+0x8000+i*2
            image[offset:offset+2]=value.to_bytes(2,'little')
        differential(lib,'synthetic-'+name,bytes(image),v,(0x131C,0xCFF))
    print('B98 original-byte/native full-state differential complete; Delete gate unchanged')


if __name__=='__main__':main()
