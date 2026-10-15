#!/usr/bin/env python3
"""Native full-state read prefix; separate Unicorn undefined-MUL flag accounting."""
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
 _fields_=[('valid',ct.c_uint)]+[(n,ct.c_uint16) for n in 'original_segment call_pc return_pc ax bx cx ds dx'.split()]
PCS=[5371, 5373, 5376, 5383, 5384, 5386, 5389, 5302, 5308, 5316, 5392, 5394, 5397, 5399, 5401, 5402, 5403, 5407, 5409, 5412, 5414, 5417]
RAW_HASH='f3bed6079585d31b69d1e93f79071ee568ab3dfe2dd1973a8c49c5094d8d3234'
RAM_HASHES={'14fb': 'b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6', '14fd': 'b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6', '1500': 'b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6', '1507': 'b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6', '1508': 'b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6', '150a': 'b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6', '150d': 'b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6', '14b6': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '14bc': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '14c4': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '1510': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '1512': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '1515': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '1517': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '1519': '52bf5b3651274b8513838c7e3072180a99a4049ae412365baf124633c5641d5a', '151a': 'eb491f02f1c317bc4d5cdf978c25edda50d33cc9ed0a06c40026ef5ed8dfd545', '151b': '9f08570bdbd720d86a44125036c02d8157204d1995cce1a1a162a13011b9e0ad', '151f': '9f08570bdbd720d86a44125036c02d8157204d1995cce1a1a162a13011b9e0ad', '1521': '9f08570bdbd720d86a44125036c02d8157204d1995cce1a1a162a13011b9e0ad', '1524': '9f08570bdbd720d86a44125036c02d8157204d1995cce1a1a162a13011b9e0ad', '1526': '9f08570bdbd720d86a44125036c02d8157204d1995cce1a1a162a13011b9e0ad', '1529': '9f08570bdbd720d86a44125036c02d8157204d1995cce1a1a162a13011b9e0ad'}
CODE_HASHES={'14fb': '4e7ca8b1abee0e0b48d592245da1470730d0baa56178fd9e3abb9f78cd037a8c', '14b6': '24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363'}
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PCS)==22
 cpus={f'{pc:04x}':[int(v,16) for v in row.split()] for pc,row in zip(PCS,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 for n,values in cpus.items():
  assert len(values)==16 and values[:2]==[0x131C,int(n,16)]
  assert values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  for pc,size in ((0x14FB,48),(0x14B6,15)):
   body=start[0xAC76+pc:0xAC76+pc+size]
   assert sha256(body).hexdigest()==CODE_HASHES[f'{pc:04x}']
   assert images[n][0x131C0+pc:0x131C0+pc+size]==body,(n,hex(pc))
 assert images['14fb'][0x2053A:0x2054C]==bytes.fromhex('490c8e2a2436161d1d0108009a442636161d')
 assert cpus['151b'][-1]==0x7206 and cpus['151f'][-1]==0x7202
 assert cpus['1529'][9:13]==[0x3F1D,6,0x11D,8] and cpus['1529'][2]==0x449A
 changed={i for i,(a,b) in enumerate(zip(images['14fb'],images['1529'])) if a!=b}
 assert changed<={0x20534,0x20535,0x20536,0x20537,0x20538,0x20539}
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints)
 full=defined=native=mismatches=0;mul_seen=False;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_system_read_14fb_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==(0x131C,terminal) and result==status and not pending,(label,pc,result)
   assert before==(cpu_values(state.cpu),memory.raw)
   if status==1:
    assert (r.valid,r.original_segment,r.call_pc,r.return_pc,r.ax,r.bx,r.cx,r.ds,r.dx)==(1,0xAF8,0x1529,0x152B,state.cpu.ax,state.cpu.bx,state.cpu.cx,state.cpu.ds,state.cpu.dx)
   else:assert not r.valid
   print(f'{label}: native-full={native}; Unicorn-full={full}; Unicorn-defined-flags={defined}; raw-flag-differences={mismatches}; stop={status}',flush=True)
   return full,defined,native,mismatches
  assert not r.valid;mapped.one(uc)
  actual,expected=cpu_values(state.cpu),mapped.values(uc)
  mul_seen=mul_seen or pc==(0x131C,0x151B)
  if mul_seen:
   # Neither state is changed. Intel leaves SF/ZF/AF/PF undefined after MUL.
   # Native full-state comparisons below still compare every flag bit.
   mismatches+=actual[-1]!=expected[-1]
   a,b=actual.copy(),expected.copy();a[-1]&=~0xD4;b[-1]&=~0xD4
   compare_cpu(a,b,label);defined+=1
  else:compare_cpu(actual,expected,label);full+=1
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   name,native_cpu,native_ram=pending.pop(0)
   compare_cpu(actual,native_cpu,name);assert memory.raw==native_ram,name;native+=1
 raise AssertionError(f'{label}: instruction bound')
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_system_read_14fb_prefix.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_system_read_14fb_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_system_read_14fb_step.restype=ct.c_int
 totals=[0,0,0,0]
 def run(label,image,values,terminal=0x1529,status=1,checkpoints=()):
  results=differential(lib,label,image,values,terminal,status,checkpoints)
  for i,n in enumerate(results):totals[i]+=n
 native_results=differential(lib,'native-prefix',images['14fb'],cpus['14fb'],0x1529,1,[(f'{pc:04x}',cpus[f'{pc:04x}'],images[f'{pc:04x}']) for pc in PCS[1:]])
 assert native_results==(16,5,21,5)
 totals=list(native_results)
 def frame(image,offset,value):
  image[0x1D160+offset:0x1D160+offset+2]=value.to_bytes(2,'little')
 for count in (0,1,0x11D,0x8000,0xFFFF):
  for size in (0,1,2,128,0xFFFF):
   image=bytearray(images['14fb']);frame(image,0x33E2,count);frame(image,0x362A,size)
   run(f'controlled-count-{count:04x}-size-{size:04x}',bytes(image),cpus['14fb'],0x1535 if count==0 else 0x1529,2 if count==0 else 1)
 for signature in (0,0xD7B0,0xD7B2,0xD7B4,0xFFFF):
  image=bytearray(images['14fb']);frame(image,0x3628,signature)
  run(f'controlled-signature-{signature:04x}',bytes(image),cpus['14fb'],0x14BE,3)
 for flags in (0x7202,0x7602,0x7AD7):
  values=cpus['14fb'].copy();values[-1]=flags;values[9]=0xABCD;values[10]=0xFFFF
  run(f'controlled-input-flags-{flags:04x}',images['14fb'],values)
 image=bytearray(images['14fb']);frame(image,0x33E4,0x8000);frame(image,0x33E6,0x2000)
 frame(image,0x3626,0xFFFF)
 run('controlled-buffer-and-handle',bytes(image),cpus['14fb'])
 for off,terminal,status in ((0x33D4,0x14BE,3),(0x33D2,0x1529,1)):
  image=bytearray(images['14fb']);frame(image,0x33E8,off)
  for displacement,value in ((0,6),(2,0xD7B3),(4,1)):frame(image,off+displacement,value)
  run(f'controlled-record-stack-alias-{off:04x}',bytes(image),cpus['14fb'],terminal,status)
 image=bytearray(images['14fb']);frame(image,0x33E8,0xFFFC);frame(image,0x33EA,0x2000)
 for off,value in ((0xFFFC,6),(0xFFFE,0xD7B3),(0,128)):
  a=0x20000+off;image[a:a+2]=value.to_bytes(2,'little')
 run('controlled-record-effective-offset-wrap',bytes(image),cpus['14fb'])
 values=cpus['1510'].copy();values[-1]&=~0x40
 run('controlled-independent-error-branch',images['1510'],values,0x1551,4)
 for ax,size,flags in ((0,1,0x7AD7),(1,1,0x7292),(0xFFFF,0xFFFF,0x7292),(0x100,0x100,0x7202)):
  image=bytearray(images['151b']);frame(image,0x362A,size)
  values=cpus['151b'].copy();values[9]=ax;values[-1]=flags
  run(f'controlled-independent-MUL-{ax:04x}-{size:04x}',bytes(image),values)
 assert totals[2]==21
 print(f'PASS native full CPU/RAM checkpoints={totals[2]} (22 pinned states); Unicorn full-state comparisons={totals[0]}; separate defined-flag/full-RAM comparisons={totals[1]}; raw undefined-flag differences={totals[3]}; DOS never executed',flush=True)
if __name__=='__main__':main()
