#!/usr/bin/env python3
"""Independent original-byte and native full-state Close-entry prefix audit."""
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
 _fields_=[('valid',ct.c_uint)]+[(n,ct.c_uint16) for n in 'original_segment call_pc return_pc ax bx'.split()]
PCS=[5265, 5267, 5271, 5302, 5308, 5316, 5274, 5276, 5279, 5282, 5284, 5286]
RAW_HASH='95afb1580bb125f7dde266a4f04e57c0eeb3c43fdaa9d60ea0ac86b0cc4f2e32'
RAM_HASHES={'1491': 'd936d445902f6169fdd50f96a06def50aa0e90801a6f4db71e1baef935177298', '1493': 'd936d445902f6169fdd50f96a06def50aa0e90801a6f4db71e1baef935177298', '1497': 'd936d445902f6169fdd50f96a06def50aa0e90801a6f4db71e1baef935177298', '14b6': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '14bc': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '14c4': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '149a': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '149c': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '149f': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '14a2': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '14a4': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355', '14a6': '216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355'}
CODE_HASH='83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858'
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PCS)==12
 cpus={f'{pc:04x}':[int(v,16) for v in row.split()] for pc,row in zip(PCS,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 body=start[0xAC76+0x1491:0xAC76+0x14C5];assert len(body)==52 and sha256(body).hexdigest()==CODE_HASH
 original_call=game[0x1DFE6+0xC4F:0x1DFE6+0xC54]
 assert original_call==bytes.fromhex('9a9114f80a')
 for n,values in cpus.items():
  assert len(values)==16 and values[:2]==[0x131C,int(n,16)]
  assert values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x131C0+0x1491:0x131C0+0x14C5]==body,n
  assert images[n][0x2A8E0+0xC4F:0x2A8E0+0xC54]==bytes.fromhex('9a91141c13'),n
 assert images['1491'][0x20544:0x2054C]==bytes.fromhex('540c8e2a2636161d')
 assert images['1491'][0x20786:0x2078C]==bytes.fromhex('0600b3d70100')
 assert images['14b6'][0x20542:0x20544]==bytes.fromhex('9a14')
 assert cpus['14a6'][9:11]==[0x3E1D,6] and cpus['14a6'][-1]==0x7202
 changed={i for i,(a,b) in enumerate(zip(images['1491'],images['14a6'])) if a!=b}
 assert changed<={0x20542,0x20543}
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);native=0;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_system_close_1491_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==(0x131C,terminal) and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status==1:
    assert (r.valid,r.original_segment,r.call_pc,r.return_pc,r.ax,r.bx)==(1,0xAF8,0x14A6,0x14A8,state.cpu.ax,state.cpu.bx)
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
 if len(sys.argv)!=3:raise SystemExit('usage: audit_system_close_1491_prefix.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_system_close_1491_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_system_close_1491_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal=0x14A6,status=1,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 run('native-loader-Close',images['1491'],cpus['1491'],checkpoints=[(f'{pc:04x}',cpus[f'{pc:04x}'],images[f'{pc:04x}']) for pc in PCS[1:]])
 def frame(image,off,value):
  a=0x1D160+off;image[a:a+2]=value.to_bytes(2,'little')
 for handle in (0,1,2,3,4,5,6,0x7FFF,0x8000,0xFFFF):
  for flags in (0x7202,0x7602,0x7AD7):
   image=bytearray(images['1491']);frame(image,0x3626,handle)
   values=cpus['1491'].copy();values[-1]=flags
   run(f'controlled-handle-{handle:04x}-flags-{flags:04x}',bytes(image),values,0x14AD if handle<=4 else 0x14A6,2 if handle<=4 else 1)
 for ax in (0,0xAB00,0xAB01,0xFFFF):
  for flags in (0x7202,0x7602,0x7AD7):
   values=cpus['1491'].copy();values[9]=ax;values[-1]=flags
   run(f'controlled-AX-{ax:04x}-flags-{flags:04x}',images['1491'],values)
 for signature in (0,0xD7B0,0xD7B2,0xD7B4,0xFFFF):
  image=bytearray(images['1491']);frame(image,0x3628,signature)
  run(f'controlled-signature-{signature:04x}',bytes(image),cpus['1491'],0x14BE,3)
 values=cpus['149a'].copy();values[-1]&=~0x40
 run('controlled-independent-failed-helper-branch',images['149a'],values,0x14B3,4)
 for off,terminal,status in ((0x33E0,0x14BE,3),(0x33E2,0x14A6,1)):
  image=bytearray(images['1491']);frame(image,0x33E8,off);frame(image,off,6);frame(image,off+2,0xD7B3)
  run(f'controlled-record-near-frame-alias-{off:04x}',bytes(image),cpus['1491'],terminal,status)
 for label,seg,off in (('alternate',0x3000,0x8000),('effective-wrap',0x2000,0xFFFE)):
  image=bytearray(images['1491']);frame(image,0x33E8,off);frame(image,0x33EA,seg)
  for delta,value in ((0,0xFFFF),(2,0xD7B3)):
   a=(seg<<4)+((off+delta)&65535);image[a:a+2]=value.to_bytes(2,'little')
  run(f'controlled-record-{label}',bytes(image),cpus['1491'])
 for sp in (0,0xFFFC):
  image=bytearray(images['1491']);values=cpus['1491'].copy();values[7]=sp
  frame(image,(sp+4)&65535,0x3626);frame(image,(sp+6)&65535,0x1D16)
  run(f'controlled-near-stack-wrap-{sp:04x}',bytes(image),values)
 assert native==11
 print(f'PASS {total} C/Unicorn full-state comparisons; {native} native full CPU/RAM checkpoints;12 pinned states; no flag masks, DOS effects or runtime tails',flush=True)
if __name__=='__main__':main()
