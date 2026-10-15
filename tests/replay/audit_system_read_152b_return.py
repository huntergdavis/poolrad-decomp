#!/usr/bin/env python3
"""Independent read return and later IOResult; native full-state/undefined-DIV distinction."""
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
PLAN=[['read-entry', 4892, 5371], ['read-dos-input', 4892, 5417], ['152b', 4892, 5419], ['152c', 4892, 5420], ['152d', 4892, 5421], ['152f', 4892, 5423], ['1531', 4892, 5425], ['1535', 4892, 5429], ['1538', 4892, 5432], ['153a', 4892, 5434], ['153c', 4892, 5436], ['153e', 4892, 5438], ['1541', 4892, 5441], ['155f', 4892, 5471], ['1560', 4892, 5472], ['0c49', 10894, 3145], ['0caa', 10894, 3242], ['io-call', 4134, 815], ['io-entry', 4892, 1186], ['io-xchg', 4892, 1188], ['io-ret', 4892, 1192], ['io-caller', 4134, 820]]
READ_PLAN=[['152b', 4892, 5419], ['152c', 4892, 5420], ['152d', 4892, 5421], ['152f', 4892, 5423], ['1531', 4892, 5425], ['1535', 4892, 5429], ['1538', 4892, 5432], ['153a', 4892, 5434], ['153c', 4892, 5436], ['153e', 4892, 5438], ['1541', 4892, 5441], ['155f', 4892, 5471], ['1560', 4892, 5472], ['0c49', 10894, 3145]]
IO_PLAN=[['io-entry', 4892, 1186], ['io-xchg', 4892, 1188], ['io-ret', 4892, 1192], ['io-caller', 4134, 820]]
RAW_HASH='0717931a6f6562b0dc7fc4ab6829f4a077f7bbd7877dd72560725d10072e6e69'
RAM_HASHES={'read-entry': '52d984ab80efa788249a6892070d836303f3601fc37dbf64637d61aa49f7de00', 'read-dos-input': 'ff30af9bf093a80cdeb8c45efa25c92b5a2e593d339b174a1fc53f02487a4819', '152b': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '152c': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '152d': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '152f': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '1531': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '1535': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '1538': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '153a': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '153c': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '153e': 'b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3', '1541': 'f17c306e3e6463aacebf026d372c9637ae5112186fa1a85c74aec70520c0bd73', '155f': 'f17c306e3e6463aacebf026d372c9637ae5112186fa1a85c74aec70520c0bd73', '1560': 'f17c306e3e6463aacebf026d372c9637ae5112186fa1a85c74aec70520c0bd73', '0c49': 'f17c306e3e6463aacebf026d372c9637ae5112186fa1a85c74aec70520c0bd73', '0caa': '7f9c0aceeb7e4e1fc7c1f30bd8ae3fe8d4c11507f2e2c151d0c0783612df4f1a', 'io-call': '06e0eec91a3acb183605e24b663ca89036eb3184fe9a5372bfaa771df97c180a', 'io-entry': 'bd41dd87a14134fa94ec9440d279a3e5729bd29ad24d9586fe96f6c717247d47', 'io-xchg': 'bd41dd87a14134fa94ec9440d279a3e5729bd29ad24d9586fe96f6c717247d47', 'io-ret': 'bd41dd87a14134fa94ec9440d279a3e5729bd29ad24d9586fe96f6c717247d47', 'io-caller': 'bd41dd87a14134fa94ec9440d279a3e5729bd29ad24d9586fe96f6c717247d47'}
CODE_HASHES={'152b': '43bdec36c4caf86049d8462bbebf5bc214c9659a6a18f3bb899edb0b268a1a9b', '04a2': 'd70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b'}
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==22
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 bodies=[]
 for pc,size in ((0x152B,56),(0x4A2,7)):
  b=start[0xAC76+pc:0xAC76+pc+size];assert sha256(b).hexdigest()==CODE_HASHES[f'{pc:04x}'];bodies.append((0x131C,pc,b))
 for pc,original,loaded in ((0xC44,'9afb14f80a','9afb141c13'),(0xCAA,'9aeb020208','9aeb022610')):
  assert game[0x1DFE6+pc:0x1DFE6+pc+5]==bytes.fromhex(original);bodies.append((0x2A8E,pc,bytes.fromhex(loaded)))
 assert start[0x8057+0x32F:0x8057+0x334]==bytes.fromhex('9aa204f80a')
 bodies.append((0x1026,0x32F,bytes.fromhex('9aa2041c13')))
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  for seg,pc,b in bodies:assert images[n][(seg<<4)+pc:(seg<<4)+pc+len(b)]==b,(n,hex(pc))
 assert images['read-entry'][0x2053A:0x2054C]==bytes.fromhex('490c8e2a2436161d1d0108009a442636161d')
 for n in ('read-dos-input','152b'):
  assert images[n][0x20538:0x2054C]==bytes.fromhex('a637490c8e2a2436161d1d0108009a442636161d')
 a,b=cpus['read-dos-input'],cpus['152b']
 assert a[9:13]==[0x3F1D,6,0x11D,8] and a[2]==0x449A
 assert b[9]==0x11D and not b[-1]&1 and all(a[i]==b[i] for i in range(16) if i not in (1,9))
 a,b=images['read-dos-input'],images['152b']
 assert sum(x!=y for x,y in zip(a,b))==145
 assert sum(x!=y for x,y in zip(a[0x449A8:0x44AC5],b[0x449A8:0x44AC5]))==126
 a,b=images['152b'],images['0c49']
 assert {i for i,(x,y) in enumerate(zip(a,b)) if x!=y}<={0x20784,0x20785}
 assert b[0x20784:0x20786]==bytes.fromhex('1d01')
 assert cpus['1531'][-1]==0x7246 and cpus['1535'][-1]==0x7242
 assert cpus['0c49'][7:10]==[0x33EC,0x37A6,0x11D]
 assert images['io-entry'][0x20494:0x20498]==bytes.fromhex('34032610')
 assert cpus['io-entry'][7:9]==[0x3334,0x33E2] and cpus['io-caller'][7:9]==[0x3338,0x33E2]
 assert images['io-entry'][0x18E26:0x18E28]==bytes(2)
 uc=mapped.emulator(images['io-call'],cpus['io-call']);mapped.one(uc)
 compare_cpu(mapped.values(uc),cpus['io-entry'],'original IOResult CALL-only frame')
 assert bytes(uc.mem_read(0,655360))==images['io-entry']
 print('PASS original IOResult CALL-only frame (excluded C);145 DOS RAM changes/126 buffer net changes excluded;two independent native inputs',flush=True)
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints)
 full=defined=native=mismatches=0;undefined=False;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_system_read_152b_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status in (1,2):assert (r.valid,r.return_segment,r.return_pc)==(1,pc[0],pc[1])
   else:assert not r.valid
   print(f'{label}: Unicorn-full={full}; DIV-defined={defined}; native-full={native}; raw-flag-differences={mismatches}; stop={status}',flush=True)
   return full,defined,native,mismatches
  assert not r.valid;mapped.one(uc)
  actual,expected=cpu_values(state.cpu),mapped.values(uc)
  if pc==(0x131C,0x1531):undefined=True
  if pc[0]==0x131C and pc[1] in (0x153A,0x1543,0x1556,0x155A,0x4A2):undefined=False
  if undefined:
   # DIV defines no status flags. Compare other bits/fields and all RAM;
   # never alter either execution. Native comparisons below are unmasked.
   mismatches+=actual[-1]!=expected[-1]
   a,b=actual.copy(),expected.copy();a[-1]&=~0x8D5;b[-1]&=~0x8D5
   compare_cpu(a,b,label);defined+=1
  else:compare_cpu(actual,expected,label);full+=1
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(actual,expected_cpu,n);assert memory.raw==expected_ram,n;native+=1
 raise AssertionError(f'{label}: instruction bound')
def divide_fault(lib,image,values,label):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 r=Report();before=cpu_values(state.cpu),memory.raw
 assert lib.pr_system_read_152b_step(ct.byref(state),ct.byref(r))==3
 assert not r.valid and before==(cpu_values(state.cpu),memory.raw)
 uc=mapped.emulator(image,values);events=[]
 def fault(u,number,_):
  events.append(number);u.emu_stop()
 uc.hook_add(unicorn.UC_HOOK_INTR,fault)
 uc.emu_start((values[0]<<4)+values[1],0x100000,count=1)
 assert events==[0],(label,events)
 print(f'{label}: original divide exception0 observed;C atomic typed fault;excluded instruction counts',flush=True)
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_system_read_152b_return.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_system_read_152b_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_system_read_152b_step.restype=ct.c_int
 totals=[0,0,0,0]
 def run(label,image,values,terminal=(0x2A8E,0xC49),status=1,checkpoints=()):
  result=differential(lib,label,image,values,terminal,status,checkpoints)
  for i,n in enumerate(result):totals[i]+=n
  return result
 result=run('native-read-return',images['152b'],cpus['152b'],checkpoints=[(n,cpus[n],images[n]) for n,_,_ in READ_PLAN[1:]])
 assert result==(10,3,13,3)
 result=run('native-later-IOResult',images['io-entry'],cpus['io-entry'],(0x1026,0x334),2,[(n,cpus[n],images[n]) for n,_,_ in IO_PLAN[1:]])
 assert result==(3,0,3,0)
 def word(image,a,value):image[a:a+2]=value.to_bytes(2,'little')
 def frame(image,off,value):word(image,0x1D160+off,value)
 for ax in (0,1,0x11D,0xFFFF):
  for size in (1,2,128,0xFFFF):
   image=bytearray(images['152b']);frame(image,0x362A,size)
   values=cpus['152b'].copy();values[9]=ax
   run(f'controlled-read-bytes-{ax:04x}-size-{size:04x}',bytes(image),values)
 for ax in (0,5,0x100,0xFFFF):
  for null in (False,True):
   image=bytearray(images['152b']);values=cpus['152b'].copy();values[9]=ax;values[-1]|=1
   if null:frame(image,0x33DE,0);frame(image,0x33E0,0)
   run(f'controlled-independent-read-error-{ax:04x}-null-{null}',bytes(image),values)
 for requested in (0x11D,0x11E):
  image=bytearray(images['152b']);frame(image,0x33DE,0);frame(image,0x33E0,0);frame(image,0x33E2,requested)
  run(f'controlled-null-result-request-{requested:04x}',bytes(image),cpus['152b'])
 for label,seg,off,terminal,status in (
  ('alternate',0x3000,0x8000,(0x2A8E,0xC49),1),
  ('saved-BP-alias',0x1D16,0x33D8,(0x2A8E,0xC49),1),
  ('return-IP-alias',0x1D16,0x33DA,(0x2A8E,0x11D),5)):
  image=bytearray(images['152b']);frame(image,0x33DE,off);frame(image,0x33E0,seg)
  run(f'controlled-result-pointer-{label}',bytes(image),cpus['152b'],terminal,status)
 image=bytearray(images['152b']);frame(image,0x33DE,0xFFFE);frame(image,0x33E0,0x2000)
 run('controlled-result-at-segment-end',bytes(image),cpus['152b'])
 for value in (0,1,0x100,0x8000,0xFFFF):
  for flags in (0x7202,0x7602,0x7AD7):
   image=bytearray(images['io-entry']);word(image,0x18E26,value)
   values=cpus['io-entry'].copy();values[-1]=flags;values[9]=0xABCD
   run(f'controlled-IOResult-word-{value:04x}-flags-{flags:04x}',bytes(image),values,(0x1026,0x334),2)
 values=cpus['io-entry'].copy();values[2]=0x1C07
 run('controlled-IOResult-return-CS-alias',images['io-entry'],values,(0,0x334),5)
 image=bytearray(images['io-entry']);values=cpus['io-entry'].copy();values[2]=0x1C07;values[7]=0x3336
 frame(image,0x3336,0x334);frame(image,0x3338,0x1026)
 run('controlled-IOResult-return-IP-alias',bytes(image),values,(0x1026,0),5)
 for label,source,sp,ret,cs,status in (
  ('read-RETF0E-wrap','1560',0xFFFC,0xC49,0x2A8E,1),
  ('IOResult-RETF-wrap','io-ret',0xFFFC,0x334,0x1026,2)):
  image=bytearray(images[source]);values=cpus[source].copy();values[7]=sp
  frame(image,sp,ret);frame(image,sp+2,cs)
  run(f'controlled-independent-{label}',bytes(image),values,(cs,ret),status)
 for label,size,dx,ax in (('zero-divisor',0,0,0x11D),('quotient-overflow',1,1,0)):
  image=bytearray(images['1531']);frame(image,0x362A,size)
  values=cpus['1531'].copy();values[12]=dx;values[9]=ax
  divide_fault(lib,bytes(image),values,label)
 assert totals[2]==16
 print(f'PASS native full CPU/RAM checkpoints={totals[2]};22 states;Unicorn full-state={totals[0]};separate DIV-defined/full-RAM={totals[1]};raw undefined-flag differences={totals[3]};1 original-CALL-only check and2 fault checks excluded;145 DOS RAM changes excluded;two independent fragments',flush=True)
if __name__=='__main__':main()
