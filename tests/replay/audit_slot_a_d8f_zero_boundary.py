#!/usr/bin/env python3
"""One focused actual zero-AL D8F case; no heavy parity matrix or downstream copy."""
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
PLAN=[('0334', 4134, 820), ('0caf', 10894, 3247), ('0cb1', 10894, 3249), ('0cb3', 10894, 3251), ('0d8f', 10894, 3471), ('0d93', 10894, 3475), ('0d94', 10894, 3476), ('0d95', 10894, 3477), ('0d99', 10894, 3481), ('0d9a', 10894, 3482), ('0d9b', 10894, 3483)]
RAW_HASH='ca7583396bd1358af2cde731f59bc908fe2b8b470380a51d10cee9fc92c6a024'
RAM_HASHES={'0334': 'f88e829fce6267a0b27c15e804c7d10bc07ee28228b966cd92baa4d2f18540fa', '0caf': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0cb1': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0cb3': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0d8f': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0d93': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0d94': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0d95': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0d99': '703158962dd166cdd91e6aab4a5522dba9cbb4f5ccc151f2a0d8a7883b6ba4e4', '0d9a': '7ccbb9acddec5ec7d4a8a310790bc17ac31b2a30504d3667806a2745503ad941', '0d9b': 'f6b2b6155100a1c7b9cd29ad6330cccadca7f2656f0e1965398b92a7436e8c83'}
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==11
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 fixture=json.loads((root/'fixture.json').read_text());name=fixture['name']
 assert Path(name).name==name and len(name)<40
 mapped.pins.checked(root/'withheld'/name,126,'a1927b99e61de81f910cae15428e8bce17fd1857197a666e605980f8ba3e8332')
 assert not any(p.name.upper()==name.upper() for p in (root/'POOLRAD').iterdir())
 code=game[0x1DFE6+0xD8F:0x1DFE6+0xDA0]
 assert len(code)==17 and sha256(code).hexdigest()=='ff578f2fa82bb3b6d20b45411dd317b4ccb5a9611253e76c0b923ee7e51bc4e3'
 loaded=bytearray(code);loaded[15:17]=(0x131C).to_bytes(2,'little')
 selector=start[0x8057+0x334:0x8057+0x356]
 assert sha256(selector).hexdigest()=='86e4cb84d29f3d8d6c422459c06563bb6218c34e1897c7d05628954c021c5014'
 branch=game[0x1DFE6+0xCAF:0x1DFE6+0xCB6]
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x2A8E0+0xD8F:0x2A8E0+0xDA0]==loaded,n
  assert images[n][0x10260+0x334:0x10260+0x356]==selector,n
  assert images[n][0x2A8E0+0xCAF:0x2A8E0+0xCB6]==branch,n
 a=images['0334'];length=a[0x20518]
 assert a[0x20519:0x20519+length].decode('ascii').replace(chr(92),'/').split('/')[-1].upper()==name.upper()
 assert cpus['0334'][9]==0x67 and cpus['0caf'][9]==0
 for first,last,count in (('0334','0caf',10),('0caf','0d8f',3)):
  uc=mapped.emulator(images[first],cpus[first])
  for _ in range(count):mapped.one(uc)
  compare_cpu(mapped.values(uc),cpus[last],'original-only '+first+'->'+last)
  assert bytes(uc.mem_read(0,655360))==images[last]
 assert images['0caf']==images['0cb1']==images['0cb3']==images['0d8f']
 assert images['0d8f'][0x193CE:0x193D0]==bytes.fromhex('6700')
 assert cpus['0d8f'][9]==0 and cpus['0d8f'][-1]==0x7246
 a,b=images['0d8f'],images['0d9b']
 assert [i for i,(x,y) in enumerate(zip(a,b)) if x!=y]==list(range(0x20544,0x20548))
 assert b[0x20544:0x2054C]==bytes.fromhex('a636161ded34161d')
 print('PASS controlled file absence,actual IOword0067/AL0;two original-only endpoints/13 instructions excluded C;11 states pinned',flush=True)
 return cpus,images
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_d8f_zero_boundary.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_slot_a_d8f_zero_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_d8f_zero_step.restype=ct.c_int
 image=images['0d8f'];memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,cpus['0d8f']))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,cpus['0d8f']);started=time.monotonic()
 for n,_,_ in PLAN[5:]:
  if time.monotonic()-started>=2:raise TimeoutError('focused six-step bound')
  r=Report();assert lib.pr_slot_a_d8f_zero_step(ct.byref(state),ct.byref(r))==0
  assert not r.dependency_valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),'C/original '+n)
  compare_cpu(cpu_values(state.cpu),cpus[n],'native '+n)
  assert memory.raw==bytes(uc.mem_read(0,len(image)))==images[n],n
 before=cpu_values(state.cpu),memory.raw;r=Report()
 assert lib.pr_slot_a_d8f_zero_step(ct.byref(state),ct.byref(r))==1
 assert before==(cpu_values(state.cpu),memory.raw)
 assert (r.dependency_valid,r.caller,r.call_pc,r.callee_segment,r.callee_pc,r.return_pc,r.caller_is_overlay)==(1,0x6F,0xD9B,0xAF8,0xB0B,0xDA0,1)
 print('PASS 6 C/Unicorn full16CPU/full655360RAM comparisons;6 controlled-original-native checkpoints;one focused case;no masks,copy/call frame or heavy matrix',flush=True)
if __name__=='__main__':main()
