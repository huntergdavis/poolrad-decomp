#!/usr/bin/env python3
"""Actual CBF pointer stores, with original-only GetMem provenance excluded."""
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
 _fields_=[('valid',ct.c_uint),('continuation_segment',ct.c_uint16),('continuation_pc',ct.c_uint16)]
PLAN=[('0cba', 10894, 3258), ('023f', 4892, 575), ('0245', 4892, 581), ('0248', 4892, 584), ('0cbf', 10894, 3263), ('0cc2', 10894, 3266), ('0cc7', 10894, 3271), ('0ccc', 10894, 3276)]
RAW_HASH='4dd8420de126889728142c9ca8893411f7b12b8abf358434d0c3a1dc7ab85466'
RAM_HASHES={'0cba': '78d948b8860a1b251fd4231e84d82ff71a019f4d1e62cc69bb49015de0818859', '023f': '74510bdad526bfc26a348a234c0dc6179e7bc9186d4542404209578811d15674', '0245': '11f73e3fbc6a401133becc0c8b1bb66f1b1526fefdb2555721934dd57220376b', '0248': 'e521c5b17873f8e5af668c8a5d15dbd0dcee21ff87df226d07c315b928451b41', '0cbf': 'e521c5b17873f8e5af668c8a5d15dbd0dcee21ff87df226d07c315b928451b41', '0cc2': 'e521c5b17873f8e5af668c8a5d15dbd0dcee21ff87df226d07c315b928451b41', '0cc7': '07d1ada1453740c5ef80c249ecd87023ed5b59b99946ac525b41d13ebfe0a850', '0ccc': 'a8f0716b57a0298e2532a388f1f99e1fcd9e62df87f784b5a4e8de38db2094a0'}
EXCLUDED_CHANGES=[119124, 119125, 132406, 132407, 132408, 132409, 132410, 132411, 132412, 132413, 132414, 132415, 132416, 132417, 132418, 132419, 281290, 281448, 281449, 281450, 281451, 281452, 281453, 281454, 281455]
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==8
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 code=game[0x1DFE6+0xCBF:0x1DFE6+0xCCC]
 assert len(code)==13 and sha256(code).hexdigest()=='891fc163483ebfe79aac69d2a2e17acf3a3af3bb9fed5c19cf822301774177dc'
 wrapper=start[0xAC76+0x23F:0xAC76+0x254]
 assert sha256(wrapper).hexdigest()=='d4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346'
 assert game[0x1DFE6+0xCBA:0x1DFE6+0xCBF]==bytes.fromhex('9a3f02f80a')
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x2A8E0+0xCBF:0x2A8E0+0xCCC]==code,n
  assert images[n][0x131C0+0x23F:0x131C0+0x254]==wrapper,n
  assert images[n][0x2A8E0+0xCBA:0x2A8E0+0xCBF]==bytes.fromhex('9a3f021c13'),n
 for a,b,count in (('0cba','023f',1),('023f','0245',3),('0248','0cbf',3)):
  uc=mapped.emulator(images[a],cpus[a])
  for _ in range(count):mapped.one(uc)
  compare_cpu(mapped.values(uc),cpus[b],f'original-only {a}->{b}')
  assert bytes(uc.mem_read(0,655360))==images[b]
 a,b=images['0245'],images['0248']
 assert [i for i,(x,y) in enumerate(zip(a,b)) if x!=y]==EXCLUDED_CHANGES
 assert len(EXCLUDED_CHANGES)==25
 assert images['023f'][0x20546:0x2054C]==bytes.fromhex('bf0c8e2a3f00')
 a,b=images['0cbf'],images['0ccc']
 assert a[0x20910:0x20914]==bytes.fromhex('08009a44')
 assert a[0x44A70:0x44A74]==bytes(4) and b[0x44A70:0x44A74]==bytes.fromhex('0800b244')
 assert [i for i,(x,y) in enumerate(zip(a,b)) if x!=y]==[0x44A70,0x44A72,0x44A73]
 assert cpus['0cbf'][7:10]==[0x33EC,0x37A6,8] and cpus['0cbf'][12]==0x44B2
 print('PASS three original-only endpoint checks/seven instructions EXCLUDED C;25 native allocator changed bytes excluded;actual returned-pointer stores pinned',flush=True)
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);native=0;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_slot_a_cbf_pointer_store_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status==1:assert (r.valid,r.continuation_segment,r.continuation_pc)==(1,0x2A8E,0xCCC)
   else:assert not r.valid
   print(f'{label}: {step} full-state comparisons;{native} native checkpoints;stop={status}',flush=True)
   return step,native
  assert not r.valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;native+=1
 raise AssertionError(f'{label}: instruction bound')
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_cbf_pointer_store.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_slot_a_cbf_pointer_store_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_cbf_pointer_store_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal=(0x2A8E,0xCCC),status=1,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 run('native-actual-CBF-pointer-stores',images['0cbf'],cpus['0cbf'],checkpoints=[(n,cpus[n],images[n]) for n,_,_ in PLAN[5:]])
 def frame(image,bp,di,es):
  a=0x1D160+((bp+0xA)&65535);image[a:a+4]=di.to_bytes(2,'little')+es.to_bytes(2,'little')
 for ax,dx in ((0,0),(0x100,0x8000),(0xABCD,0xEF01),(0xFFFF,0xFFFF)):
  for flags in (0x7202,0x7602,0x7AD7):
   for es,di in ((0x5000,0x1000),(0x1D16,0x8000),(0,0xFF38)):
    image=bytearray(images['0cbf']);frame(image,0x37A6,di,es)
    values=cpus['0cbf'].copy();values[9]=ax;values[12]=dx;values[-1]=flags
    run(f'controlled-pointer-{es:04x}:{di:04x}-payload-{dx:04x}:{ax:04x}-flags-{flags:04x}',bytes(image),values)
 for label,es,di in (('source-frame-alias',0x1D16,0x36E8),('same-physical-alias-different-segment',0x1D17,0x36D8)):
  image=bytearray(images['0cbf']);frame(image,0x37A6,di,es)
  values=cpus['0cbf'].copy();values[9]=0xABCD;values[12]=0xEF01
  run('controlled-'+label,bytes(image),values)
 image=bytearray(images['0cbf']);frame(image,0xFFF6,0x1000,0x5000)
 values=cpus['0cbf'].copy();values[8]=0xFFF6
 run('controlled-BP-relative-LES-offset-wrap',bytes(image),values)
 for di,terminal in ((0xFF37,0xCC2),(0xFF35,0xCC7)):
  image=bytearray(images['0cbf']);frame(image,0x37A6,di,0x5000)
  run(f'controlled-unsupported-word-crossing-DI-{di:04x}',bytes(image),cpus['0cbf'],(0x2A8E,terminal),2)
 values=cpus['0cbf'].copy();values[8]=0xFFF4
 run('controlled-unsupported-four-byte-LES-crossing',images['0cbf'],values,(0x2A8E,0xCBF),2)
 values=cpus['0cc7'].copy();values[12]=0xFFFF
 run('controlled-independent-segment-word-store',images['0cc7'],values)
 assert native==3
 print(f'PASS {total} C/Unicorn full-state comparisons;{native} native checkpoints;8 same-run states;three original-only endpoints/seven instructions and25allocator bytes excluded;no masks or allocator join',flush=True)
if __name__=='__main__':main()
