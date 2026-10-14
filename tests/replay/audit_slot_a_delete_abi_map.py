#!/usr/bin/env python3
"""Map controlled native Delete ABI; the C Delete gate remains blocked."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/replay'))
import audit_slot_a_string_0356_abi as prefix
from audit_slot_a_string_0356_abi import CPU,State,Report,NAMES,REGS,cpu_values,compare_cpu
sys.path.insert(0,str(ROOT/'tests/recovered'))
import check_slot_a_u6f_196a_resident_native as pins
RAM_SIZE=655360

RAM_HASHES = {
    "196a-natural": "03832ea833e47411c07ada59cfdbeb9703748e07c58b490688c963fa0cc1dfec",
    "196a-controlled": "2653e11eaa25bd5d1901b674e93e5f7f4046aa0fac41f09c91c59da7dd40e444",
    "03fc": "ffed4537737c02607b84c395b070d75feb0001670518ed85a2a2a73a0d1a8501",
    "0cbc": "0413cdde392f828e2c92cdcc55c249695c63e1bab5ccde3efaf288be83505a20",
    "0cbd": "055082d79ec1c3e6fe1e3c40e5ab8d4a59dd24efc757c3d17ea293ea3a031c89",
    "0b57-prefix": "2e962045a16d61db30b1e4cc3131e7cc2007a6129d4ed5bd1a6fc25902446417",
    "0ce1": "2ec0bc22c132ce94c05d7725d11fa4164902354e70f465668de8889ba3113d71",
    "0b57-suffix": "f3aaceade4a262c01431e26f7e648db889e951b4f849cd0cd34c6fdd40cb3811",
    "0cfb": "964ee2c4d92e59eb2d1782b1f71877d524e70bd59a031258af2d701932cb072f",
    "0b98": "c92c00df6c402cec499ce1712d544305470000309a119ac55c80b8b8aada372f",
    "0cff": "4cc07ba112309e0a8b85d3ae8d236e6857ba28210bc0862cec2a44ad75466756",
    "0b25": "8e62c5ce7e00f23a00f1fe01f0209cd3ece38c5abd7004c0989015fda3b9d331",
    "0b3d": "8e62c5ce7e00f23a00f1fe01f0209cd3ece38c5abd7004c0989015fda3b9d331",
    "0b3e": "ee0ba5bf1ee6aabd33e734dcb3b3b016604d27ef87668393d2fc732842275c6c",
    "0b44": "9b9098f7607d4915d69cdf9b6a24b7e422b9390f186f598a8070ef4dbe62488c",
    "0d0c": "9b9098f7607d4915d69cdf9b6a24b7e422b9390f186f598a8070ef4dbe62488c",
    "0d0f": "9b9098f7607d4915d69cdf9b6a24b7e422b9390f186f598a8070ef4dbe62488c",
    "0401": "9b9098f7607d4915d69cdf9b6a24b7e422b9390f186f598a8070ef4dbe62488c"
}

def inputs(root):
    raw=(root/'native.log').read_bytes()
    assert sha256(raw).hexdigest()=='ac6e7f6e47fb8e1758687e20b90737dee67e2d4ad54c6a11176b5d018ba53c30'
    text=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
    rows=re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)",text)
    assert len(rows)==len(RAM_HASHES)==18
    cpus={name:[int(v,16) for v in row.split()] for name,row in zip(RAM_HASHES,rows)}
    images={name:pins.checked(root/('ram-'+name+'.bin'),RAM_SIZE,digest) for name,digest in RAM_HASHES.items()}
    for name,values in cpus.items():
        assert len(values)==16
        assert values==[json.loads((root/('cpu-'+name+'.json')).read_text())[n] for n in NAMES]
        cs=0x2A8E if name.startswith('196a') else (0x1026 if name in ('03fc','0401') else 0x131C)
        assert values[:2]==[cs,int(name.split('-')[0],16)]
    natural,controlled=(images[n] for n in ('196a-natural','196a-controlled'))
    changes={i for i,(a,b) in enumerate(zip(natural,controlled)) if a!=b}
    allowed=set(range(0x20D16,0x20D18))|set(range(0x20DE2,0x20DE6))
    assert len(changes)==5 and changes<=allowed
    assert cpus['196a-natural']==cpus['196a-controlled']
    assert controlled[0x20D16:0x20D18]==b'\x01A'
    assert controlled[0x20DE2:0x20DE6]==b'\x03ABA'
    changed=lambda a,b:{i for i,(x,y) in enumerate(zip(images[a],images[b])) if x!=y}
    assert changed('03fc','0cbc')=={0x205F2,0x205F3}
    assert changed('0cbc','0cbd')=={0x205F0,0x205F1}
    assert changed('0b3d','0b3e')=={0x20802}
    assert changed('0b3e','0b44')=={0x20803,0x20804}
    assert len(changed('03fc','0401'))==31
    assert images['03fc'][0x20802:0x20806]==b'\x03ABA'
    assert images['0401'][0x20802:0x20806]==b'\x02BAA'  # trailing byte retained
    word=lambda label,off:int.from_bytes(images[label][0x1D160+off:0x1D160+off+2],'little')
    assert [word('03fc',p) for p in range(0x3496,0x349E,2)]==[1,1,0x36A2,0x1D16]
    assert [word('0cbc',p) for p in range(0x3492,0x349E,2)]==[0x401,0x1026,1,1,0x36A2,0x1D16]
    assert [word('0b57-prefix',p) for p in range(0x3280,0x3290,2)]==[
        0xCE1,0x131C,0,1,0x36A2,0x1D16,0x3390,0x1D16]
    assert [word('0b57-suffix',p) for p in range(0x327C,0x328C,2)]==[
        0xCFB,0x131C,255,2,0x36A2,0x1D16,0x3290,0x1D16]
    assert [word('0b98',p) for p in range(0x3284,0x3290,2)]==[
        0xCFF,0x131C,0x3290,0x1D16,0x3390,0x1D16]
    assert [word('0b25',p) for p in range(0x3282,0x3290,2)]==[
        0xD0C,0x131C,255,0x36A2,0x1D16,0x3390,0x1D16]
    return cpus,images


def code(root,ram):
    pins.check_code(root,ram)  # full executable/save + original/loaded 0356
    start=(root/'POOLRAD/START.EXE').read_bytes()
    for pc,off,size,digest in (
        (0xCBC,0xB932,86,'14be20a0c6b74485ec3dcc683266dc07bc2c6e130d9ddcb2f83434b926996a33'),
        (0xB57,0xB7CD,65,'18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa'),
        (0xB98,0xB80E,44,'c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64'),
        (0xB25,0xB79B,36,pins.COPY_HASH),
        (0xC10,0xB886,43,'11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58'),
    ):
        body=start[off:off+size]
        assert sha256(body).hexdigest()==digest
        assert ram[0x131C0+pc:0x131C0+pc+size]==body
    assert sha256(start[0x8453:0x8458]).hexdigest()=='a5def57c24a53296a5350790c360ecfb2e5a61c7e1f5981cc27f78918529077f'
    assert sha256(ram[0x10260+0x3FC:0x10260+0x401]).hexdigest()=='14b42bc81a603890a934028783a90ef4c631b709a6b3e42d77c9de802a76c416'


def emulator(image,values):
    uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map(0,0x100000);uc.mem_write(0,image)
    for register,value in zip(REGS,values):uc.reg_write(register,value)
    return uc


def values(uc):return [uc.reg_read(r)&65535 for r in REGS]


def one(uc):
    pc=(uc.reg_read(REGS[0]),uc.reg_read(REGS[1]))
    for _ in range(65537):
        uc.emu_start((uc.reg_read(REGS[0])<<4)+uc.reg_read(REGS[1]),0x100000,count=1)
        if (uc.reg_read(REGS[0]),uc.reg_read(REGS[1]))!=pc:return
    raise AssertionError('original instruction did not finish')


def controlled_native(cpus,images):
    uc=emulator(images['03fc'],cpus['03fc'])
    pending=list(RAM_HASHES)[3:]
    seen=0
    for steps in range(1000):
        cs,ip=values(uc)[:2]
        if (cs,ip)==(0x1026,0x401):
            assert not pending
            print(f'controlled Delete: {steps} original-byte instructions; {seen} native CPU/full-RAM checkpoints match (no C body claim)')
            return
        assert (cs,ip)==(0x1026,0x3FC) or (cs==0x131C and (
            0xCBC<=ip<0xD12 or 0xB25<=ip<0xB49 or 0xB57<=ip<0xBC4))
        one(uc)
        if pending and values(uc)[:2]==cpus[pending[0]][:2]:
            label=pending.pop(0)
            compare_cpu(values(uc),cpus[label],label)
            assert bytes(uc.mem_read(0,RAM_SIZE))==images[label],label+': native RAM mismatch'
            seen+=1
    raise AssertionError('Delete trace bound exhausted')


def blocked(lib,image,cpu):
    memory=ct.create_string_buffer(image,len(image))
    state=State(CPU(**dict(zip(NAMES,cpu))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
    report=Report()
    assert lib.pr_slot_a_string_0356_step(ct.byref(state),ct.byref(report))==2
    assert cpu_values(state.cpu)==cpu and memory.raw==image
    assert (report.dependency_valid,report.caller,report.call_pc,report.callee_segment,
            report.callee_pc,report.return_pc,report.caller_is_overlay)==(1,0x802,0x3FC,0xAF8,0xCBC,0x401,0)


def nonpositive(lib,cpus,images):
    entry=cpus['0cbc'];stack=entry[6]<<4;sp=entry[7]
    expected=[0xCBC,0xCBD,0xCBF,0xCC3,0xCC7,0xD0C,0xD0E,0xD0F]
    for count in (0,0xFFFF,0x8000):
        image=bytearray(images['0cbc'])
        image[stack+sp+4:stack+sp+6]=count.to_bytes(2,'little')
        image[stack+sp+8:stack+sp+12]=b'\xff'*4  # must not dereference
        uc=emulator(bytes(image),entry);trace=[]
        for _ in range(32):
            cs,ip=values(uc)[:2]
            if (cs,ip)==(0x1026,0x401):break
            assert cs==0x131C
            trace.append(ip);one(uc)
        assert trace==expected
        actual=values(uc)
        assert actual[7]==(sp+12)&65535
        for i,name in enumerate(NAMES):
            if name not in ('cs','ip','sp','flags'):assert actual[i]==entry[i],name
        out=bytes(uc.mem_read(0,RAM_SIZE))
        changed={i for i,(a,b) in enumerate(zip(image,out)) if a!=b}
        assert changed<=set(range(stack+sp-2,stack+sp))
        blocked(lib,bytes(image),cpus['03fc'])
        print(f'emulator-only signed count {count:04X}: eight-instruction no-dereference return; C gate unchanged')


def main():
    if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_delete_abi_map.py SHARED_0356_LIBRARY IGNORED_NATIVE_ROOT')
    if unicorn.__version__!='2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library,root=(Path(arg).resolve(strict=True) for arg in sys.argv[1:])
    if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
        raise SystemExit('library and private inputs must stay in this clone resources/')
    cpus,images=inputs(root)
    for image in images.values():code(root,image)
    lib=ct.CDLL(str(library))
    lib.pr_slot_a_string_0356_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)]
    lib.pr_slot_a_string_0356_step.restype=ct.c_int
    checkpoints={'actual-Delete-gate':((0x1026,0x3FC,False),cpus['03fc'],images['03fc'])}
    prefix.differential(lib,'controlled original caller prefix',images['196a-controlled'],cpus['196a-controlled'],2,checkpoints)
    blocked(lib,images['03fc'],cpus['03fc'])
    controlled_native(cpus,images)
    nonpositive(lib,cpus,images)
    print('Delete ABI mapping complete; C Delete remains explicitly unsupported')


if __name__=='__main__':main()
