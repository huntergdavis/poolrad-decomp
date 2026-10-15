#!/usr/bin/env python3
"""Actual selector-return CAF context to external GetMem gate; no allocator execution."""
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
PLAN=[('0caa', 10894, 3242), ('0334', 4134, 820), ('0caf', 10894, 3247), ('0cb1', 10894, 3249), ('0cb6', 10894, 3254), ('0cb9', 10894, 3257), ('0cba', 10894, 3258)]
RAW_HASH='08265abbc8ad732886593dfea948cda18223331ff6580d1cb6498972bb20ebc8'
RAM_HASHES={'0caa': '9ad8f6e43985fe1c97a85f4e24df4de0d41732256c2ec67476a8c241ac349de3', '0334': 'befadef18bf513fc8cc275e0031c8cb7b710d3e67397a1e7779f14eff313bdca', '0caf': '329be84d4ca69bce633a209f1af59555af98b24f97679dd8604d11e75f718751', '0cb1': '329be84d4ca69bce633a209f1af59555af98b24f97679dd8604d11e75f718751', '0cb6': '329be84d4ca69bce633a209f1af59555af98b24f97679dd8604d11e75f718751', '0cb9': '329be84d4ca69bce633a209f1af59555af98b24f97679dd8604d11e75f718751', '0cba': '641c398f51ef0288b4623de51ecb959fe13331b77a76a408cc1a0c2c0b578ab8'}
LOADED_HASH='ac945d9129af9a75f2e11fc2d7b6a232eb6cafd8d108bdbf740981325f0c3f6d'
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==7
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 code=game[0x1DFE6+0xCAF:0x1DFE6+0xCBF]
 assert len(code)==16 and sha256(code).hexdigest()=='6a6980b142121eccd959ff3ea69319c89a5a3e843ab69af6f26cc16721cffae1'
 loaded=bytearray(code);loaded[14:16]=(0x131C).to_bytes(2,'little');assert sha256(loaded).hexdigest()==LOADED_HASH
 selector=start[0x8057+0x334:0x8057+0x356]
 assert sha256(selector).hexdigest()=='86e4cb84d29f3d8d6c422459c06563bb6218c34e1897c7d05628954c021c5014'
 assert game[0x1DFE6+0xCAA:0x1DFE6+0xCAF]==bytes.fromhex('9aeb020208')
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x2A8E0+0xCAF:0x2A8E0+0xCBF]==loaded,n
  assert images[n][0x10260+0x334:0x10260+0x356]==selector,n
  assert images[n][0x2A8E0+0xCAA:0x2A8E0+0xCAF]==bytes.fromhex('9aeb022610'),n
 assert images['0334'][0x20542:0x2054C]==bytes.fromhex('a637af0c8e2aed34161d')
 assert cpus['0334'][7:10]==[0x3338,0x33E2,0]
 uc=mapped.emulator(images['0334'],cpus['0334'])
 for _ in range(11):mapped.one(uc)
 compare_cpu(mapped.values(uc),cpus['0caf'],'original-only actual selector return')
 assert bytes(uc.mem_read(0,655360))==images['0caf']
 assert cpus['0caf'][7:10]==[0x33EC,0x37A6,1]
 a,b=images['0caf'],images['0cba']
 assert {i for i,(x,y) in enumerate(zip(a,b)) if x!=y}=={0x2054A,0x2054B}
 assert b[0x2054A:0x2054C]==bytes([0x3F,0])
 print('PASS one original-only 11-instruction selector endpoint;excluded C counts;no unmerged C dependency',flush=True)
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);native=0;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_slot_a_loader_caf_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status==1:
    assert (r.dependency_valid,r.caller,r.call_pc,r.callee_segment,r.callee_pc,r.return_pc,r.caller_is_overlay)==(1,0x6F,0xCBA,0xAF8,0x23F,0xCBF,1)
   else:assert not r.dependency_valid
   print(f'{label}: {step} full-state comparisons; {native} native checkpoints; stop={status}',flush=True)
   return step,native
  assert not r.dependency_valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;native+=1
 raise AssertionError(f'{label}: instruction bound')
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_loader_caf_gate.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_slot_a_loader_caf_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_loader_caf_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal=None,status=None,checkpoints=()):
  nonlocal total,native
  if terminal is None:
   terminal=(0x2A8E,0xCBA if values[9]&255 else 0xD8F);status=1 if values[9]&255 else 2
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 run('native-actual-selector-return-CAF',images['0caf'],cpus['0caf'],checkpoints=[(n,cpus[n],images[n]) for n,_,_ in PLAN[3:]])
 for low in (0,1,0x7F,0x80,0xFF):
  for high in (0,0xAB):
   for flags in (0x7202,0x7602,0x7AD7):
    values=cpus['0caf'].copy();values[9]=(high<<8)|low;values[-1]=flags
    run(f'controlled-AX-{values[9]:04x}-flags-{flags:04x}',images['0caf'],values)
 for sp in (0,2,0x8000):
  values=cpus['0caf'].copy();values[7]=sp
  run(f'controlled-stack-{sp:04x}',images['0caf'],values)
 values=cpus['0cb9'].copy();values[9]=0xABCD
 run('controlled-independent-PUSH-entire-AX',images['0cb9'],values,(0x2A8E,0xCBA),1)
 values=cpus['0caf'].copy();values[7]=1
 run('controlled-unsupported-split-PUSH',images['0caf'],values,(0x2A8E,0xCB9),3)
 for zf in (0,0x40):
  values=cpus['0cb1'].copy();values[9]=0;values[-1]=(values[-1]&~0x40)|zf
  run(f'controlled-JNE-uses-flags-{zf:02x}',images['0cb1'],values,(0x2A8E,0xD8F if zf else 0xCBA),2 if zf else 1)
 assert native==4
 print(f'PASS {total} C/Unicorn full-state comparisons;{native} native checkpoints;7 same-run states;one original-only selector endpoint excluded;no flag masks or allocator execution',flush=True)
if __name__=='__main__':main()
