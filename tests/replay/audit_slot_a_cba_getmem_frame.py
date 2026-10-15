#!/usr/bin/env python3
"""Two independent actual CAF GetMem call/return fragments; allocator gap excluded."""
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
 _fields_=[('dependency_valid',ct.c_uint),('call_is_near',ct.c_uint),('call_pc',ct.c_uint16),('callee_segment',ct.c_uint16),('callee_pc',ct.c_uint16),('return_pc',ct.c_uint16)]
PLAN=[('0cba', 10894, 3258), ('023f', 4892, 575), ('0240', 4892, 576), ('0242', 4892, 578), ('0245', 4892, 581), ('0248', 4892, 584), ('0249', 4892, 585), ('024b', 4892, 587), ('0cbf', 10894, 3263)]
RAW_HASH='d37b7f81c8ac7ecc5f866a376cbed7d4b7c5c4f8936258e3608dfd8ea72458e5'
RAM_HASHES={'0cba': 'faf24e60219a63671c2b44eb532a28edc1d803b69f7506f4b3592a1714358a2c', '023f': 'e67d7dca4163ea8c12ec26a7933f0d17c104d771459877e739b1ce5110712e53', '0240': 'ce25e587715c7a5081ab377f5e9686bf8dc61ded5d8594aa460f3833ce72550b', '0242': 'ce25e587715c7a5081ab377f5e9686bf8dc61ded5d8594aa460f3833ce72550b', '0245': 'ce25e587715c7a5081ab377f5e9686bf8dc61ded5d8594aa460f3833ce72550b', '0248': 'd6ef2d838eaeb89cc4875cfe9ec4fae591f067e719e34957a6d8fc83af2c0356', '0249': 'd6ef2d838eaeb89cc4875cfe9ec4fae591f067e719e34957a6d8fc83af2c0356', '024b': 'd6ef2d838eaeb89cc4875cfe9ec4fae591f067e719e34957a6d8fc83af2c0356', '0cbf': 'd6ef2d838eaeb89cc4875cfe9ec4fae591f067e719e34957a6d8fc83af2c0356'}
EXCLUDED_CHANGES=[119124, 119125, 132406, 132407, 132408, 132409, 132410, 132411, 132412, 132413, 132414, 132415, 132416, 132417, 132418, 132419, 281290, 281448, 281449, 281450, 281451, 281452, 281453, 281454, 281455]
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==9
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 wrapper=start[0xAC76+0x23F:0xAC76+0x254]
 assert len(wrapper)==21 and sha256(wrapper).hexdigest()=='d4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346'
 assert game[0x1DFE6+0xCBA:0x1DFE6+0xCBF]==bytes.fromhex('9a3f02f80a')
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x131C0+0x23F:0x131C0+0x254]==wrapper,n
  assert images[n][0x2A8E0+0xCBA:0x2A8E0+0xCBF]==bytes.fromhex('9a3f021c13'),n
 for n,_,_ in PLAN[1:]:assert images[n][0x20546:0x2054C]==bytes.fromhex('bf0c8e2a3f00'),n
 for n,_,_ in PLAN[2:]:assert images[n][0x20544:0x20546]==bytes.fromhex('a637'),n
 a,b=images['0245'],images['0248']
 assert [i for i,(x,y) in enumerate(zip(a,b)) if x!=y]==EXCLUDED_CHANGES
 assert len(EXCLUDED_CHANGES)==25
 assert cpus['0248'][7:10]==[0x33E4,0x33E4,8] and cpus['0248'][12]==0x44B2
 assert b[0x20542:0x20544]==bytes.fromhex('4802')
 assert cpus['0cbf'][7:10]==[0x33EC,0x37A6,8] and cpus['0cbf'][12]==0x44B2
 assert b==images['0249']==images['024b']==images['0cbf']
 print('PASS nine native states;actual far/near frames;25 changed bytes across native-only allocator interval EXCLUDED from C coverage',flush=True)
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);native=0;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_slot_a_cba_getmem_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status==1:
    assert (r.dependency_valid,r.call_is_near,r.call_pc,r.callee_segment,r.callee_pc,r.return_pc)==(1,1,0x245,0xAF8,0x308,0x248)
   else:assert not r.dependency_valid
   print(f'{label}: {step} full-state comparisons;{native} native checkpoints;stop={status}',flush=True)
   return step,native
  assert not r.dependency_valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;native+=1
 raise AssertionError(f'{label}: instruction bound')
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_cba_getmem_frame.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_slot_a_cba_getmem_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_cba_getmem_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal,status,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 run('native-CBA-entry-prefix',images['0cba'],cpus['0cba'],(0x131C,0x245),1,[(n,cpus[n],images[n]) for n,_,_ in PLAN[1:5]])
 run('native-INDEPENDENT-actual-first-core-return',images['0248'],cpus['0248'],(0x2A8E,0xCBF),4,[(n,cpus[n],images[n]) for n,_,_ in PLAN[6:]])
 def word(image,off,v):
  a=0x1D160+off;image[a:a+2]=v.to_bytes(2,'little')
 for size in (0,1,0x3F,0xFFFF):
  for sp in (0,0x33EA):
   for flags in (0x7202,0x7602,0x7AD7):
    image=bytearray(images['0cba']);word(image,sp,size)
    values=cpus['0cba'].copy();values[7]=sp;values[9]=0xABCD;values[-1]=flags
    run(f'controlled-size-{size:04x}-SP-{sp:04x}-flags-{flags:04x}',bytes(image),values,(0x131C,0x245),1)
 for ax,dx in ((0,0),(0xFFFF,0x1234),(0xCAFE,0xBEEF)):
  for flags in (0x7202,0x7602,0x7AD7):
   values=cpus['0248'].copy();values[9]=ax;values[12]=dx;values[-1]=flags
   terminal=(0x131C,0xE2) if flags&1 else (0x2A8E,0xCBF)
   run(f'controlled-independent-return-{dx:04x}:{ax:04x}-flags-{flags:04x}',images['0248'],values,terminal,3 if flags&1 else 4)
 image=bytearray(images['0248']);word(image,0x33E6,0x1234);word(image,0x33E8,0x5678)
 run('controlled-unknown-actual-RETF-target',bytes(image),cpus['0248'],(0x5678,0x1234),6)
 image=bytearray(images['024b']);word(image,0xFFFC,0xCBF);word(image,0xFFFE,0x2A8E)
 values=cpus['024b'].copy();values[7]=0xFFFC
 run('controlled-RETF2-SP-wrap',bytes(image),values,(0x2A8E,0xCBF),4)
 for sp in (1,2,3):
  values=cpus['0cba'].copy();values[7]=sp
  run(f'controlled-unsupported-cross-boundary-far-frame-SP-{sp}',images['0cba'],values,(0x2A8E,0xCBA),5)
 image=bytearray(images['0242']);word(image,0,0xABCD)
 values=cpus['0242'].copy();values[8]=0xFFFA
 run('controlled-BP-relative-argument-offset-wrap',bytes(image),values,(0x131C,0x245),1)
 assert native==7
 print(f'PASS {total} C/Unicorn full-state comparisons;{native} native checkpoints;9 same-run states;allocator interval and25changed bytes excluded;no masks or continuous prefix-to-return join',flush=True)
if __name__=='__main__':main()
