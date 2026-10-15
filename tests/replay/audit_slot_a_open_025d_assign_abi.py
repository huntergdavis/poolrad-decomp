#!/usr/bin/env python3
"""Reached025D/Assign full-state ABI; Reset and DOS remain unsupported."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import time
import unicorn
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/replay'))
import audit_slot_a_delete_abi_map as mapped
from audit_slot_a_string_0356_abi import CPU,State,Report,NAMES,cpu_values,compare_cpu
RAM_HASHES = {
    "0c26": "4c3d652961ac52ee8b9d5d9478bfe3da211f0dab285fd74c061ad5e6eaaed4b4",
    "025d": "2cf69e67b644430b7c93f60042fa963c97fa8325f4d76460cd666b0083c14644",
    "025e": "8c230fe74eb078f2e566de5bebe191985b99d58144f4aeeabf7c60f0fbebb1ba",
    "0260": "8c230fe74eb078f2e566de5bebe191985b99d58144f4aeeabf7c60f0fbebb1ba",
    "0263": "8c230fe74eb078f2e566de5bebe191985b99d58144f4aeeabf7c60f0fbebb1ba",
    "0266": "8c230fe74eb078f2e566de5bebe191985b99d58144f4aeeabf7c60f0fbebb1ba",
    "0267": "9f22e776917811e42b2b2c8251ee97a79df8db0e9d55b867e7201f9f9e7a034a",
    "0268": "cd0f03c1ceab942c2c100c5483b9a00912387d60490d9cd50fb62ab63157d7c7",
    "026b": "cd0f03c1ceab942c2c100c5483b9a00912387d60490d9cd50fb62ab63157d7c7",
    "026c": "b4aa1775739de18d9b983b1e5c43e31947afffd19166674d5cd91755adaecbe8",
    "026d": "d91a5df399ae7472bae48e6151a00e61ed7a22a7420d23fe32d11d3dedc40f88",
    "0270": "d91a5df399ae7472bae48e6151a00e61ed7a22a7420d23fe32d11d3dedc40f88",
    "0271": "fdc5e1ad4a44b5a7882677b9382070ae3738dae94cdb5c0d720503d56fe6f9bb",
    "0271-0b25": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b26": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b28": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b2a": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b2e": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b32": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b36": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b37": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b39": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b3d": "c554a50cff05b5c4b840f5509f430b9f72eddda8d6358cddf551fa8002e85312",
    "0271-0b3e": "b5279384f34b256eecd67f4955f1b961ceadb6cb2a7f0a4c3a1ff413e1edebd8",
    "0271-0b40": "b5279384f34b256eecd67f4955f1b961ceadb6cb2a7f0a4c3a1ff413e1edebd8",
    "0271-0b42": "b5279384f34b256eecd67f4955f1b961ceadb6cb2a7f0a4c3a1ff413e1edebd8",
    "0271-0b44": "441d9f2ddbd7016ee4d82843a9e88f8dfaf84154bd4f56f637ef00fdca3f6ff8",
    "0271-0b46": "441d9f2ddbd7016ee4d82843a9e88f8dfaf84154bd4f56f637ef00fdca3f6ff8",
    "0276": "441d9f2ddbd7016ee4d82843a9e88f8dfaf84154bd4f56f637ef00fdca3f6ff8",
    "0279": "441d9f2ddbd7016ee4d82843a9e88f8dfaf84154bd4f56f637ef00fdca3f6ff8",
    "027a": "441d9f2ddbd7016ee4d82843a9e88f8dfaf84154bd4f56f637ef00fdca3f6ff8",
    "027b": "c140ae394fdcb6c065b0ac6a97b231efb14fbdcc4b49a816d29ba0ed8a3111e4",
    "027e": "c140ae394fdcb6c065b0ac6a97b231efb14fbdcc4b49a816d29ba0ed8a3111e4",
    "027f": "c140ae394fdcb6c065b0ac6a97b231efb14fbdcc4b49a816d29ba0ed8a3111e4",
    "0280": "9714526379884b98918609272697327ad5350dc37d3b5630dfbc9c5ae42e9489",
    "0283": "9714526379884b98918609272697327ad5350dc37d3b5630dfbc9c5ae42e9489",
    "0284": "9714526379884b98918609272697327ad5350dc37d3b5630dfbc9c5ae42e9489",
    "0284-callee": "f89bf65a74b147689caff013fca7a11369de72fb51df51514d31ad268150725d",
    "0289": "d9de187dcc4aa88a94dd856f5cb8f35b5eee2fea812f3a53433540a52cd6516f",
    "028c": "d9de187dcc4aa88a94dd856f5cb8f35b5eee2fea812f3a53433540a52cd6516f",
    "028d": "d9de187dcc4aa88a94dd856f5cb8f35b5eee2fea812f3a53433540a52cd6516f",
    "028e": "1ed8395ff1e529734734017a5b9c4e20ff84c0fa174c8566c8a27947917e3cb4",
    "028f": "1fddae91402589c5f06215ec8972bd696ba956371b5c7d62b22a2cb1770d26e3",
    "02eb": "6f1c3dae1960fb32594d4bd60760ecc4effd07f9a67dcf09b7d2294dc763ef5f",
    "02ec": "5cba6430a43c243c561c70683f54f18adf8c3bbb7adefb5be83613289bc31ff6",
    "02ee": "5cba6430a43c243c561c70683f54f18adf8c3bbb7adefb5be83613289bc31ff6",
    "02f2": "5cba6430a43c243c561c70683f54f18adf8c3bbb7adefb5be83613289bc31ff6",
    "02f5": "5cba6430a43c243c561c70683f54f18adf8c3bbb7adefb5be83613289bc31ff6",
    "02f6": "12bca5cc7f36efa7ecf28f8547bae83d5b9a409d5a932a9b7143591b91eea6fd",
    "02f7": "ac581f8a39d9e12053fbf44bf177408208a3922acb53ceb0b091e8a3bac27ac3",
    "02fa": "ac581f8a39d9e12053fbf44bf177408208a3922acb53ceb0b091e8a3bac27ac3",
    "02fb": "6af4bdbfb7605d4ee6012c66f46eb210ce9417cea1610eccfe0fa2948084acb0",
    "02fc": "ec38a86464ed13df5c60ae4857aaa8a2291dd5622bb84e9e728243bcbbd4020b",
    "02ff": "ec38a86464ed13df5c60ae4857aaa8a2291dd5622bb84e9e728243bcbbd4020b",
    "0300": "acb0d803dd5563c9c87c18a491a85a47b4b3523778a62c7a41420288bb1e0f6c",
    "0300-callee": "d626c3bd330d75b948c6aadf07956f429756fbd08466f21d3f0b80ee50c4d5c1",
    "0305": "f62131065baa8d905f4a3415ad32aa88c3bfb9e6355792c92b731c929529e83a",
    "0309": "f62131065baa8d905f4a3415ad32aa88c3bfb9e6355792c92b731c929529e83a",
    "030a": "f62131065baa8d905f4a3415ad32aa88c3bfb9e6355792c92b731c929529e83a",
    "030b": "c258ba5cfe40d1780a0ab73a4f40be04ce8670312c6e5c17678052a2792393ba",
    "030e": "c258ba5cfe40d1780a0ab73a4f40be04ce8670312c6e5c17678052a2792393ba",
    "030f": "c258ba5cfe40d1780a0ab73a4f40be04ce8670312c6e5c17678052a2792393ba",
    "0310": "c258ba5cfe40d1780a0ab73a4f40be04ce8670312c6e5c17678052a2792393ba",
    "13e2": "6fade482454677f9e4937230bbf0493d1c22e5e48643dc55bf34eb9c4657b5d5",
    "13e4": "6fade482454677f9e4937230bbf0493d1c22e5e48643dc55bf34eb9c4657b5d5",
    "13e5": "b87448661f8396465018a2e8accbca40de2f9b3f7e18c03b988a6dedb35052ef",
    "13e9": "b87448661f8396465018a2e8accbca40de2f9b3f7e18c03b988a6dedb35052ef",
    "13ed": "b87448661f8396465018a2e8accbca40de2f9b3f7e18c03b988a6dedb35052ef",
    "13ee": "b87448661f8396465018a2e8accbca40de2f9b3f7e18c03b988a6dedb35052ef",
    "13f0": "b87448661f8396465018a2e8accbca40de2f9b3f7e18c03b988a6dedb35052ef",
    "13f1": "b87448661f8396465018a2e8accbca40de2f9b3f7e18c03b988a6dedb35052ef",
    "13f4": "b87448661f8396465018a2e8accbca40de2f9b3f7e18c03b988a6dedb35052ef",
    "13f5": "18457219817a8eb0831de74371978effca642fdcb7efd9cab6149378290f1174",
    "13f7": "18457219817a8eb0831de74371978effca642fdcb7efd9cab6149378290f1174",
    "13fa": "18457219817a8eb0831de74371978effca642fdcb7efd9cab6149378290f1174",
    "13fc": "45591e32aafb8b41ec9cefad6e2f5cc370c0ff4aab0afb82933e0b86bcfbf8a1",
    "13fd": "45591e32aafb8b41ec9cefad6e2f5cc370c0ff4aab0afb82933e0b86bcfbf8a1",
    "13ff": "45591e32aafb8b41ec9cefad6e2f5cc370c0ff4aab0afb82933e0b86bcfbf8a1",
    "1403": "45591e32aafb8b41ec9cefad6e2f5cc370c0ff4aab0afb82933e0b86bcfbf8a1",
    "1405": "45591e32aafb8b41ec9cefad6e2f5cc370c0ff4aab0afb82933e0b86bcfbf8a1",
    "1407": "45591e32aafb8b41ec9cefad6e2f5cc370c0ff4aab0afb82933e0b86bcfbf8a1",
    "1409": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "140b": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "140c": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "140d": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "0315": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "0319": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "031a": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "031b": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "031e": "865ac10e2a9c9ee2d378188027c20c89fefd074fe02ddce7f7e83b04751bcaac",
    "031f": "ae69a61ea0d7b30e2bd218f0d0bb57393d7796965812a0208e5a7e7fc8564f45"
}
RESIDENT_HASHES = {'0x25d': 'eeea6613012d3db99c7ad85788fdd3ff5b9888d7cc6701e9ce7a612195210dc6', '0x2eb': '141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474'}

def word(image,off):
    return int.from_bytes(image[0x1D160+off:0x1D160+off+2],'little')


def inputs(root):
    raw=(root/'native.log').read_bytes()
    assert sha256(raw).hexdigest()=='d3c2758bed18bfb91ca2212f96a49fefd77819758880ee3425595473240af1d3'
    clean=re.sub(chr(27)+r'\[[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
    rows=re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)",clean)
    assert len(rows)==len(RAM_HASHES)==91
    cpus={n:[int(v,16) for v in row.split()] for n,row in zip(RAM_HASHES,rows)}
    images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
    start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
    game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
    mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
    bodies={}
    for pc,size,calls in ((0x25D,142,(0x271,0x284,0x2C9,0x2D7)),(0x2EB,107,(0x300,0x310,0x31F,0x32A,0x32F))):
        original=start[0x8057+pc:0x8057+pc+size]
        assert sha256(original).hexdigest()==RESIDENT_HASHES[hex(pc)]
        relocated=bytearray(original)
        for call in calls:
            off=call-pc
            assert original[off]==0x9A and original[off+3:off+5]==(0xAF8).to_bytes(2,'little')
            relocated[off+3:off+5]=(0x131C).to_bytes(2,'little')
        bodies[pc]=bytes(relocated)
    assign=start[0xC058:0xC086]
    assert sha256(assign).hexdigest()=='94e00b1b9e01bc846824e0f7eb23bc8d3c80c28b089995524ce99045af95ffa7'
    copy=start[0xB79B:0xB7BF]
    assert sha256(copy).hexdigest()==mapped.pins.COPY_HASH
    call=game[0x1DFE6+0xC26:0x1DFE6+0xC2B]
    assert call[:3]==bytes.fromhex('9a5d02') and call[3:]==(0x802).to_bytes(2,'little')
    loaded_call=call[:3]+(0x1026).to_bytes(2,'little')
    for n,values in cpus.items():
        assert len(values)==16 and values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
        if n=='0c26': target=(0x2A8E,0xC26)
        elif n.endswith('-callee'):target=(0x131C,0xB25)
        elif '-' in n:target=(0x131C,int(n.split('-')[1],16))
        else:
            pc=int(n,16);target=(0x131C if pc>=0x13E2 else 0x1026,pc)
        assert values[:2]==list(target)
        image=images[n]
        assert image[0x2A8E0+0xC26:0x2A8E0+0xC2B]==loaded_call
        for pc,body in bodies.items():assert image[0x10260+pc:0x10260+pc+len(body)]==body
        assert image[0x131C0+0x13E2:0x131C0+0x1410]==assign
        assert image[0x131C0+0xB25:0x131C0+0xB49]==copy
    assert [word(images['0c26'],0x33DE+2*i) for i in range(7)]==[0x3626,0x1D16,0,0x35EE,0x1D16,0x34ED,0x1D16]
    assert cpus['0260'][8]==0x33D8 and cpus['0266'][14]==0x34ED
    assert cpus['0279'][14]==0x35EE
    assert images['0276'][0x2050E]==23 and images['0289'][0x204E5]==19
    record=images['0315'][0x20430:0x204B0]
    assert sha256(record).hexdigest()=='3a79efb3f12aca41c6414e392e436b9b92d203d962798973e43ae490df7dbd8c'
    assert record[:4]==bytes.fromhex('0000b0d7') and record[4:48]==bytes(44)
    assert record[48:72]==images['0315'][0x204B1:0x204C8]+bytes(1)
    assert [word(images['031f'],0x32CA+2*i) for i in range(3)]==[0x80,0x32D0,0x1D16]
    assert sum(a!=b for a,b in zip(images['13e2'],images['0315']))==46
    assert sum(a!=b for a,b in zip(images['0c26'],images['031f']))==144
    return cpus,images


def differential(lib,uc,label,image,values,terminal=(0x1026,0x31F),checkpoints=()):
    memory=ct.create_string_buffer(image,len(image))
    state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
    uc.mem_write(0,image)
    for reg,value in zip(mapped.REGS,values):uc.reg_write(reg,value)
    pending=list(checkpoints);seen=own=0;started=time.monotonic()
    for step in range(300):
        pc=(state.cpu.cs,state.cpu.ip)
        if time.monotonic()-started>=10:raise TimeoutError(f'{label}: case deadline at {pc}')
        if pc==terminal and terminal==(0x1026,0x315):
            assert not pending
            print(f'{label}: {step} full-state comparisons; independent real Assign return',flush=True)
            return step,own,seen
        before_cpu,before_ram=cpu_values(state.cpu),memory.raw
        report=Report();status=lib.pr_slot_a_open_025d_step(ct.byref(state),ct.byref(report))
        if status:
            assert pc==terminal==(0x1026,0x31F) and status==1 and not pending,(label,pc,status)
            assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
            assert (report.dependency_valid,report.caller,report.call_pc,report.callee_segment,
                    report.callee_pc,report.return_pc,report.caller_is_overlay)==(1,0x802,0x31F,0xAF8,0x1410,0x324,0)
            print(f'{label}: {step} full-state comparisons ({own} new); {seen} native checkpoints; Reset blocked',flush=True)
            return step,own,seen
        assert not report.dependency_valid
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
        assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
        own+=pc==(0x2A8E,0xC26) or (pc[0]==0x1026 and (0x25D<=pc[1]<=0x28F or pc[1]==0x310)) or (pc[0]==0x131C and 0x13E2<=pc[1]<=0x140D)
        if pending and cpu_values(state.cpu)[:2]==pending[0][1][:2]:
            n,expected_cpu,expected_ram=pending.pop(0)
            compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;seen+=1
    raise AssertionError(f'{label}: instruction bound')


def main():
    if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_open_025d_assign_abi.py LIBRARY NATIVE_ROOT')
    if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
    library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
        raise SystemExit('library and inputs must stay in this clone resources/')
    cpus,images=inputs(root);lib=ct.CDLL(str(library))
    lib.pr_slot_a_open_025d_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_open_025d_step.restype=ct.c_int
    uc=mapped.emulator(images['0c26'],cpus['0c26']);total=own=native=0
    def run(*args,**kwargs):
        nonlocal total,own,native
        n,o,c=differential(lib,uc,*args,**kwargs);total+=n;own+=o;native+=c
    run('natural025D and Assign',images['0c26'],cpus['0c26'],checkpoints=[(n,cpus[n],images[n]) for n in list(RAM_HASHES)[1:]])
    values=cpus['0c26'].copy();values[-1]=0x7ED7;values[9]=0xABCD
    run('synthetic incoming AH/DF/flags',images['0c26'],values)
    for length in (0,1,40,255):
        image=bytearray(images['0c26'])
        for off,n in ((0x5000,length),(0x5200,3)):
            image[0x1D160+off:0x1D160+off+n+1]=bytes([n])+bytes(0x61+i%26 for i in range(n))
        image[0x20548:0x2054C]=b''.join(v.to_bytes(2,'little') for v in (0x5000,0x1D16))
        image[0x20544:0x20548]=b''.join(v.to_bytes(2,'little') for v in (0x5200,0x1D16))
        run(f'synthetic025D candidate length{length}',bytes(image),cpus['0c26'])
    for length in (0,79,80,255):
        image=bytearray(images['13e2'])
        image[0x22160:0x22160+length+1]=bytes([length])+bytes(17*i%256 for i in range(length))
        image[0x20428:0x20430]=b''.join(v.to_bytes(2,'little') for v in (0x5000,0x1D16,0x6000,0x1D16))
        run(f'synthetic independent Assign length{length}',bytes(image),cpus['13e2'],terminal=(0x1026,0x315))
    image=bytearray(images['13e2']);values=cpus['13e2'].copy();values[7]=0xFFFC
    image[0x2D15C:0x2D160]=b''.join(v.to_bytes(2,'little') for v in (0x315,0x1026))
    image[0x1D160:0x1D168]=b''.join(v.to_bytes(2,'little') for v in (0x3350,0x1D16,0x6000,0x1D16))
    run('synthetic independent wrapped Assign frame',bytes(image),values,terminal=(0x1026,0x315))
    for source in (0x6004,0x6030):
        image=bytearray(images['13e2']);image[0x1D160+source:0x1D160+source+5]=bytes((4,17,34,51,68))
        image[0x20428:0x20430]=b''.join(v.to_bytes(2,'little') for v in (source,0x1D16,0x6000,0x1D16))
        run(f'synthetic Assign live source alias{source:04X}',bytes(image),cpus['13e2'],terminal=(0x1026,0x315))
    image=bytearray(images['1407']);values=cpus['1407'].copy()
    values[11]=4;values[13]=0x5000;values[14]=0x5001
    image[0x22160:0x22164]=bytes((17,34,51,68))
    run('synthetic independent forward-overlap REP',bytes(image),values,terminal=(0x1026,0x315))
    print(f'025D/Assign audit passed: {total} full-state C/original comparisons ({own} new, {total-own} unchanged helper); '
          f'{native} distinct post-input native checkpoints;91 untouched rows; Reset/DOS effects remain blocked',flush=True)


if __name__=='__main__':main()
