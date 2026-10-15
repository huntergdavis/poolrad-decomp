#!/usr/bin/env python3
"""Independent post-DOS Close cleanup with same-run original return-frame pins."""
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
from audit_slot_a_string_0356_abi import CPU,State,NAMES,cpu_values,compare_cpu
class Report(ct.Structure):
 _fields_=[('valid',ct.c_uint),('return_segment',ct.c_uint16),('return_pc',ct.c_uint16)]
PLAN=[['1491', 4892, 5265], ['14a6', 4892, 5286], ['14a8', 4892, 5288], ['14ad', 4892, 5293], ['14b3', 4892, 5299], ['0c54', 10894, 3156]]
RAW_HASH='e64f033dc2ed33dda733926d192cde94672c96bee907b04fd97e3c37fc131f42'
RAM_HASHES={'1491': 'b65c59183da7065e889821d3ccc933895d98553805471463aee1243a16cea95b', '14a6': '151c0111232d37b3b8116d5333a2a1d896ff453392c9088ea308fa92627435fb', '14a8': 'af3b6bfdf55881ec20d04c8b7124f2228fcf8b13b82cedd10d0b5719ef932c46', '14ad': 'af3b6bfdf55881ec20d04c8b7124f2228fcf8b13b82cedd10d0b5719ef932c46', '14b3': '4ad38419534e7efaea3121836138d04c14e19463b78000b89e2bb54ce7e567c3', '0c54': '4ad38419534e7efaea3121836138d04c14e19463b78000b89e2bb54ce7e567c3'}
CODE_HASH='83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858'
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==6
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 body=start[0xAC76+0x1491:0xAC76+0x14C5];assert len(body)==52 and sha256(body).hexdigest()==CODE_HASH
 assert game[0x1DFE6+0xC4F:0x1DFE6+0xC54]==bytes.fromhex('9a9114f80a')
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x131C0+0x1491:0x131C0+0x14C5]==body,n
  assert images[n][0x2A8E0+0xC4F:0x2A8E0+0xC54]==bytes.fromhex('9a91141c13'),n
 for n in ('1491','14a6','14a8'):
  assert images[n][0x20544:0x2054C]==bytes.fromhex('540c8e2a2636161d'),n
  assert cpus[n][7:9]==[0x33E4,0x37A6]
 a,b=cpus['14a6'],cpus['14a8']
 assert a[9]==0x3E1D and a[10]==6 and b[9]==0x3E01 and not b[-1]&1
 assert all(a[i]==b[i] for i in range(16) if i not in (1,9))
 a,b=images['14a6'],images['14a8']
 assert sum(x!=y for x,y in zip(a,b))==24
 assert a[0x20786:0x20806]==b[0x20786:0x20806]
 assert images['14a8'][0x20786:0x2078C]==bytes.fromhex('0600b3d70100')
 a,b=images['14a8'],images['0c54']
 assert {i for i,(x,y) in enumerate(zip(a,b)) if x!=y}=={0x20788}
 assert b[0x20786:0x2078C]==bytes.fromhex('0600b0d70100')
 assert cpus['0c54'][7:10]==[0x33EC,0x37A6,0x3E01]
 print('PASS same-run actual DOS input/return and real loader frame;24 DOS RAM changes excluded from C',flush=True)
 return cpus,images
def differential(lib,label,image,values,terminal=(0x2A8E,0xC54),status=1,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);native=0;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_system_close_14a8_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status==1:assert (r.valid,r.return_segment,r.return_pc)==(1,0x2A8E,0xC54)
   else:assert not r.valid
   print(f'{label}: {step} full-state comparisons; {native} native checkpoints; typed stop={status}',flush=True)
   return step,native
  assert not r.valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;native+=1
 raise AssertionError(f'{label}: instruction bound')
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_system_close_14a8_cleanup.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_system_close_14a8_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_system_close_14a8_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal=(0x2A8E,0xC54),status=1,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 run('native-post-DOS-cleanup',images['14a8'],cpus['14a8'],checkpoints=[(n,cpus[n],images[n]) for n in ('14ad','14b3','0c54')])
 for carry in (0,1):
  for ax in (0,1,0x100,0x8000,0xFFFF):
   for flags in (0x7202,0x7602,0x7AD7):
    values=cpus['14a8'].copy();values[9]=ax;values[-1]=(flags&~1)|carry
    run(f'controlled-CF-{carry}-AX-{ax:04x}-flags-{flags:04x}',images['14a8'],values)
 for label,seg,off,terminal,status in (
  ('alternate',0x3000,0x8000,(0x2A8E,0xC54),1),
  ('effective-wrap',0x2000,0xFFFF,(0x2A8E,0xC54),1),
  ('return-IP-alias',0x1D16,0x33E2,(0x2A8E,0xD7B0),3),
  ('return-CS-alias',0x1D16,0x33E4,(0xD7B0,0xC54),3)):
  values=cpus['14a8'].copy();values[3]=seg;values[14]=off
  run(f'controlled-cleanup-{label}',images['14a8'],values,terminal,status)
 values=cpus['14a8'].copy();values[2]=0x1C12;values[9]=0;values[-1]|=1
 run('controlled-error-store-return-CS-alias',images['14a8'],values,(0,0xC54),3)
 values=cpus['14a8'].copy();values[2]=0x1C36;values[9]=0xFFFF;values[-1]|=1
 run('controlled-error-store-file-handle-alias',images['14a8'],values)
 values=cpus['14a8'].copy();values[2]=0x1C36;values[14]=0x3624;values[9]=0xABCD;values[-1]|=1
 run('controlled-error-and-cleanup-same-word',images['14a8'],values)
 values=cpus['14b3'].copy();values[7]=0xFFFC
 image=bytearray(images['14b3']);image[0x1D160+0xFFFC:0x1D160+0x10000]=bytes.fromhex('540c8e2a')
 run('controlled-independent-RETF4-stack-wrap',bytes(image),values)
 values=cpus['14ad'].copy();values[9]=0xABCD
 run('controlled-independent-cleanup-word-store',images['14ad'],values)
 image=bytearray(images['14b3']);image[0x20544:0x20548]=bytes(4)
 run('controlled-independent-unknown-return',bytes(image),cpus['14b3'],(0,0),3)
 assert native==3
 print(f'PASS {total} C/Unicorn full-state comparisons; {native} native full checkpoints;6 same-run states;24 DOS RAM changes excluded; independent post-DOS input; no flag masks or continuous DOS join',flush=True)
if __name__=='__main__':main()
