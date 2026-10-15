#!/usr/bin/env python3
"""Caller0292 CPU fragments; Assign/Reset/display effects remain gated."""
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
from audit_slot_a_string_0356_abi import CPU,State,Report,NAMES,cpu_values,compare_cpu
PLANS=[[['0292', 4134, 658], ['0295', 4134, 661], ['0299', 4134, 665], ['02ad', 4134, 685], ['02b1', 4134, 689], ['02b9', 4134, 697], ['02bd', 4134, 701], ['02bf', 4134, 703], ['02c2', 4134, 706], ['02c3', 4134, 707], ['02c4', 4134, 708], ['02c7', 4134, 711], ['02c8', 4134, 712], ['02c9', 4134, 713]], [['02ce', 4134, 718], ['02d1', 4134, 721], ['02d2', 4134, 722], ['02d3', 4134, 723], ['02d6', 4134, 726], ['02d7', 4134, 727]], [['02dc', 4134, 732], ['02df', 4134, 735], ['02e2', 4134, 738], ['02e5', 4134, 741], ['02e7', 4134, 743], ['02e8', 4134, 744], ['0c2b', 10894, 3115]]]
STOPS=[['0292', 4134, 658], ['0295', 4134, 661], ['0299', 4134, 665], ['02ad', 4134, 685], ['02b1', 4134, 689], ['02b9', 4134, 697], ['02bd', 4134, 701], ['02bf', 4134, 703], ['02c2', 4134, 706], ['02c3', 4134, 707], ['02c4', 4134, 708], ['02c7', 4134, 711], ['02c8', 4134, 712], ['02c9', 4134, 713], ['assign-entry', 4892, 5090], ['02ce', 4134, 718], ['02d1', 4134, 721], ['02d2', 4134, 722], ['02d3', 4134, 723], ['02d6', 4134, 726], ['02d7', 4134, 727], ['reset-entry', 4892, 5136], ['02dc', 4134, 732], ['02df', 4134, 735], ['02e2', 4134, 738], ['02e5', 4134, 741], ['02e7', 4134, 743], ['02e8', 4134, 744], ['0c2b', 10894, 3115]]
RAW_HASH='dd5f4aa13963e9d4031f9e1cc5428cb0acda2b262b84937e7d78c132fbd2399a'
RAM_HASHES={'0292': '03242a7832e635c9998064b9a7767c5bba8a99f12e82f8bd5595d647536d9fc4', '0295': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '0299': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02ad': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02b1': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02b9': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02bd': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02bf': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02c2': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02c3': '60b6376b7bfb1dad0bf5e0dd91736a745e1fe2391a8f5a0fc43a26680f0d8490', '02c4': 'dab0cc990d3812ceedf73dfe3b40a9563d88726e995786326a2b307133a8c4cb', '02c7': 'dab0cc990d3812ceedf73dfe3b40a9563d88726e995786326a2b307133a8c4cb', '02c8': '2b10f876348093813050f6f2110e80bb3cebe3eca39a061dc4ce6f4db28dba8e', '02c9': 'bef09fb2c72a756f53cc5abc9c569535fff38f84193e1e1471c9b5a941cfcdd1', 'assign-entry': '10b20fc74bf568d85d9e70f09a62d6ad9e939db6b38b1ede715a42634c98efc8', '02ce': '4f3285c7cb411a7bff6bf7a5515618eb0ac54c996ad767be348c433c24760f61', '02d1': '4f3285c7cb411a7bff6bf7a5515618eb0ac54c996ad767be348c433c24760f61', '02d2': '4f3285c7cb411a7bff6bf7a5515618eb0ac54c996ad767be348c433c24760f61', '02d3': '4f3285c7cb411a7bff6bf7a5515618eb0ac54c996ad767be348c433c24760f61', '02d6': '4f3285c7cb411a7bff6bf7a5515618eb0ac54c996ad767be348c433c24760f61', '02d7': 'e8a61ba60df6678b0540f9c47f22e57600372777f247be3bc723ed553b654067', 'reset-entry': '47bb19390edeb50a03535a9f31a7dcf736ccf2beca2e14e0209294242518f13b', '02dc': '4939c37ba3b6b6df69b567cb519232819a5f4749dbd2b9a27425bd075b084b77', '02df': '4939c37ba3b6b6df69b567cb519232819a5f4749dbd2b9a27425bd075b084b77', '02e2': '47b919e967b97f45b4cf7bce810e184fc84c1b6d30dbd6c1acd0caa159447fa0', '02e5': '47b919e967b97f45b4cf7bce810e184fc84c1b6d30dbd6c1acd0caa159447fa0', '02e7': '47b919e967b97f45b4cf7bce810e184fc84c1b6d30dbd6c1acd0caa159447fa0', '02e8': '47b919e967b97f45b4cf7bce810e184fc84c1b6d30dbd6c1acd0caa159447fa0', '0c2b': '47b919e967b97f45b4cf7bce810e184fc84c1b6d30dbd6c1acd0caa159447fa0'}
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(STOPS)==29
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(STOPS,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,RAM_HASHES[n]) for n,_,_ in STOPS}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 tail=start[0x8057+0x292:0x8057+0x2EB]
 assert sha256(tail).hexdigest()=='ddfe7addfa0e224bf10c01d330559c947c5e8644ffe2a28affbe121ae82b9737'
 body=start[0x82B4:0x8342];assert sha256(body).hexdigest()=='eeea6613012d3db99c7ad85788fdd3ff5b9888d7cc6701e9ce7a612195210dc6'
 loaded=bytearray(body)
 for call in (0x271,0x284,0x2C9,0x2D7):
  i=call-0x25D;assert loaded[i]==0x9A and loaded[i+3:i+5]==(0xAF8).to_bytes(2,'little');loaded[i+3:i+5]=(0x131C).to_bytes(2,'little')
 bodies=[(0x1026,0x25D,bytes(loaded))]
 for pc,n,digest in [(0x13E2,46,'94e00b1b9e01bc846824e0f7eb23bc8d3c80c28b089995524ce99045af95ffa7'),(0x1410,101,'18b5680c7d5ea7bfdee3902b7a11279f62216c07fbff9c692fb8820363b481f0')]:
  b=start[0xAC76+pc:0xAC76+pc+n];assert sha256(b).hexdigest()==digest;bodies.append((0x131C,pc,b))
 call=game[0x1DFE6+0xC26:0x1DFE6+0xC2B];assert call==bytes.fromhex('9a5d020208')
 bodies.append((0x2A8E,0xC26,call[:3]+(0x1026).to_bytes(2,'little')))
 for n,cs,ip in STOPS:
  values=cpus[n];assert len(values)==16 and values[:2]==[cs,ip]
  assert values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  for seg,pc,b in bodies:assert images[n][(seg<<4)+pc:(seg<<4)+pc+len(b)]==b,(n,hex(pc))
 def words(n,off,count):
  b=images[n];return [int.from_bytes(b[0x1D160+off+2*i:0x1D160+off+2*i+2],'little') for i in range(count)]
 assert words('0292',0x33DA,9)==[0xC2B,0x2A8E,0x3626,0x1D16,0,0x35EE,0x1D16,0x34ED,0x1D16]
 assert images['0292'][0x2050E]==23 and images['0292'][0x204E5]==19
 assert words('02c9',0x337C,4)==[0x33AE,0x1D16,0x3626,0x1D16]
 assert words('assign-entry',0x3378,6)==[0x2CE,0x1026,0x33AE,0x1D16,0x3626,0x1D16]
 assert words('02d7',0x337E,3)==[1,0x3626,0x1D16]
 assert words('reset-entry',0x337A,5)==[0x2DC,0x1026,1,0x3626,0x1D16]
 assert images['02ce'][0x20786:0x2078C]==bytes.fromhex('0000b0d70000')
 assert images['02dc'][0x20786:0x2078C]==bytes.fromhex('0600b3d70100')
 assert images['02ce'][0x2078C:0x20806]==images['02dc'][0x2078C:0x20806]
 assert cpus['0c2b'][7:10]==[0x33EC,0x37A6,1] and cpus['0c2b'][-1]==0x7246
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image));state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);seen=0;started=time.monotonic()
 for step in range(100):
  pc=(state.cpu.cs,state.cpu.ip)
  if time.monotonic()-started>=10:raise TimeoutError(f'{label}: deadline at {pc}')
  before_cpu,before_ram=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_slot_a_open_0292_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
   fields=(r.dependency_valid,r.caller,r.call_pc,r.callee_segment,r.callee_pc,r.return_pc,r.caller_is_overlay)
   if status in (1,2):assert fields==(1,0x802,pc[1],0xAF8,0x13E2 if status==1 else 0x1410,pc[1]+5,0)
   elif status==3:assert fields==(1,0x802,0x2AA,0x802,0x193,0x2AD,0)
   else:assert not r.dependency_valid
   print(f'{label}: {step} complete CPU/RAM comparisons; {seen} native checkpoints; typed stop {status}',flush=True)
   return step,seen
  assert not r.dependency_valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;seen+=1
 raise AssertionError(f'{label}: instruction bound')

def native_call_frames(cpus,images):
 # C blocks before these CALLs. Original-byte emulator frame checks are not C effects.
 for before,after in [('02c9','assign-entry'),('02d7','reset-entry')]:
  uc=mapped.emulator(images[before],cpus[before]);mapped.one(uc)
  compare_cpu(mapped.values(uc),cpus[after],f'original CALL-only {before}')
  assert bytes(uc.mem_read(0,655360))==images[after]
 print('PASS two original-call-only entry frame comparisons; excluded from C totals',flush=True)
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_open_0292_continuation.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);native_call_frames(cpus,images);lib=ct.CDLL(str(library))
 lib.pr_slot_a_open_0292_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_open_0292_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal,status,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 for plan,status in zip(PLANS,(1,2,5)):
  first=plan[0][0];last=plan[-1];run(f'native-{first}',images[first],cpus[first],(last[1],last[2]),status,[(n,cpus[n],images[n]) for n,_,_ in plan[1:]])
 for ax in (0,1,0x80,0xFF,0xAB00,0xAB01):
  for mode in (0,1,0x100,0x101):
   image=bytearray(images['0292']);image[0x20542:0x20544]=mode.to_bytes(2,'little')
   values=cpus['0292'].copy();values[9]=ax;values[-1]=0x7AD7
   if ax&255:terminal,status=(0x1026,0x2C9),1
   elif mode&255:terminal,status=(0x2A8E,0xC2B),5
   else:terminal,status=(0x1026,0x2AA),3
   run(f'controlled-AX-{ax:04x}-mode-{mode:04x}',bytes(image),values,terminal,status)
 # Independently seeded after-message CPU path; no message effect is supplied.
 image=bytearray(images['02ad']);image[0x204E4]=0;image[0x20542:0x20544]=bytes(2)
 run('controlled-independent-retry-tail',bytes(image),cpus['02ad'],(0x1026,0x289),4)
 for saved in (0,1,0x80,0xFF):
  image=bytearray(images['02dc']);image[0x204E4]=saved
  values=cpus['02dc'].copy();values[9]=0xABCD;values[-1]=0x7AD7
  run(f'controlled-independent-return-AL-{saved:02x}',bytes(image),values,(0x2A8E,0xC2B),5)
 image=bytearray(images['02e8']);values=cpus['02e8'].copy();values[7]=0xFFFC
 image[0x2D15C:0x2D160]=bytes.fromhex('2b0c8e2a')
 run('controlled-RETF0E-stack-wrap',bytes(image),values,(0x2A8E,0xC2B),5)
 image=bytearray(images['0292']);image[0x2053E:0x20542]=bytes.fromhex('00800020')
 run('controlled-alternate-file-record-pointer',bytes(image),cpus['0292'],(0x1026,0x2C9),1)
 assert native==24
 print(f'PASS {total} C full-state comparisons; {native} native C checkpoints;29 states;2 separate original-CALL frame checks; three independent fragments; runtime effects excluded',flush=True)
if __name__=='__main__':main()
