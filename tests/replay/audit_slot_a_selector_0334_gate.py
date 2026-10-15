#!/usr/bin/env python3
"""Independent selector post-IOResult store/return gate with actual native frames."""
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
PLAN=[['0caa', 10894, 3242], ['io-entry', 4892, 1186], ['0334', 4134, 820], ['0337', 4134, 823], ['033c', 4134, 828], ['033e', 4134, 830], ['0342', 4134, 834], ['0348', 4134, 840], ['034a', 4134, 842], ['034d', 4134, 845], ['0350', 4134, 848], ['0352', 4134, 850], ['0353', 4134, 851], ['0caf', 10894, 3247]]
RAW_HASH='0be309e7e33f6d686825a1bba0ed77fdb43d33fbf0cf8388dfa60c267a717233'
RAM_HASHES={'0caa': '9c1abde12ea5323e1be2c404dd233935eabf592d9147d9ef63139134bddb0970', 'io-entry': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '0334': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '0337': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '033c': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '033e': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '0342': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '0348': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '034a': 'c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5', '034d': '491a882722a183d2cdfba9d08b16da536fd9a4626ba3d5c8de4d0e831a0d3991', '0350': '491a882722a183d2cdfba9d08b16da536fd9a4626ba3d5c8de4d0e831a0d3991', '0352': '491a882722a183d2cdfba9d08b16da536fd9a4626ba3d5c8de4d0e831a0d3991', '0353': '491a882722a183d2cdfba9d08b16da536fd9a4626ba3d5c8de4d0e831a0d3991', '0caf': '491a882722a183d2cdfba9d08b16da536fd9a4626ba3d5c8de4d0e831a0d3991'}
CALLER_HASH='86e4cb84d29f3d8d6c422459c06563bb6218c34e1897c7d05628954c021c5014'
IO_HASH='d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b'
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==14
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 caller=start[0x8057+0x334:0x8057+0x356];assert len(caller)==34 and sha256(caller).hexdigest()==CALLER_HASH
 io=start[0xAC76+0x4A2:0xAC76+0x4A9];assert sha256(io).hexdigest()==IO_HASH
 assert game[0x1DFE6+0xCAA:0x1DFE6+0xCAF]==bytes.fromhex('9aeb020208')
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x10260+0x334:0x10260+0x356]==caller,n
  assert images[n][0x131C0+0x4A2:0x131C0+0x4A9]==io,n
  assert images[n][0x2A8E0+0xCAA:0x2A8E0+0xCAF]==bytes.fromhex('9aeb022610'),n
 for n,_,_ in PLAN[1:]:
  assert images[n][0x20542:0x2054C]==bytes.fromhex('a637af0c8e2aed34161d'),n
 assert images['io-entry'][0x20494:0x20498]==bytes.fromhex('34032610')
 assert cpus['0334'][7:10]==[0x3338,0x33E2,0] and images['0334'][0x20518]==23
 assert images['0334'][0x193CE:0x193D0]==bytes(2)
 a,b=images['0334'],images['0caf']
 assert {i for i,(x,y) in enumerate(zip(a,b)) if x!=y}=={0x20541}
 assert b[0x20541]==1 and cpus['0caf'][7:10]==[0x33EC,0x37A6,1]
 uc=mapped.emulator(images['io-entry'],cpus['io-entry'])
 for _ in range(3):mapped.one(uc)
 compare_cpu(mapped.values(uc),cpus['0334'],'original-only IOResult provenance')
 assert bytes(uc.mem_read(0,655360))==images['0334']
 print('PASS one original-only three-instruction IOResult endpoint check;excluded C;actual selector frame returnsCAF',flush=True)
 return cpus,images
def differential(lib,label,image,values,terminal=(0x2A8E,0xCAF),status=1,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);native=0;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_slot_a_selector_0334_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status==1:assert (r.valid,r.return_segment,r.return_pc)==(1,0x2A8E,0xCAF)
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
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_selector_0334_gate.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_slot_a_selector_0334_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_selector_0334_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal=(0x2A8E,0xCAF),status=1,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 run('native-selector0334-caller',images['0334'],cpus['0334'],checkpoints=[(n,cpus[n],images[n]) for n,_,_ in PLAN[3:]])
 for ax in (0,1,0x100,0x8000,0xFFFF):
  for length in (0,1,23,0x80,0xFF):
   for flags in (0x7202,0x7602,0x7AD7):
    image=bytearray(images['0334']);image[0x20518]=length
    values=cpus['0334'].copy();values[9]=ax;values[-1]=flags
    run(f'controlled-IOword-{ax:04x}-length-{length:02x}-flags-{flags:04x}',bytes(image),values)
 def frame(image,off,value):
  a=0x1D160+off;image[a:a+2]=value.to_bytes(2,'little')
 def caller_frame(image,bp):
  frame(image,bp,0x37A6);frame(image,(bp+2)&65535,0xCAF);frame(image,(bp+4)&65535,0x2A8E)
  image[0x1D160+((bp-0x2A)&65535)]=23
 for label,bp,ds,ax,terminal,status in (
  ('global-alias-name',0x33D8,0x1BB4,0,(0x2A8E,0xCAF),1),
  ('global-alias-return-IP',0x33DC,0x1BB7,0,(0x2A8E,0),3),
  ('global-alias-result-and-saved-BP',0x33DF,0x1BB7,0x100,(0x2A8E,0xCAF),1)):
  image=bytearray(images['0334']);caller_frame(image,bp)
  values=cpus['0334'].copy();values[8]=bp;values[2]=ds;values[9]=ax
  run(f'controlled-{label}',bytes(image),values,terminal,status)
 image=bytearray(images['0334']);caller_frame(image,0xFFFA)
 values=cpus['0334'].copy();values[8]=0xFFFA
 run('controlled-full-caller-stack-wrap',bytes(image),values)
 image=bytearray(images['0334']);image[0x20544:0x20548]=bytes(4)
 run('controlled-unknown-real-return',bytes(image),cpus['0334'],(0,0),3)
 image=bytearray(images['034d']);image[0x20541]=0xA5
 values=cpus['034d'].copy();values[9]=0xAB00
 run('controlled-independent-local-byte-reload-AH',bytes(image),values)
 assert native==11
 print(f'PASS {total} C/Unicorn full-state comparisons;{native} native full checkpoints;14 same-run states;one original-only IOResult endpoint excluded;no masks,IOResult/DOS injection or read-to-selector join',flush=True)
if __name__=='__main__':main()
