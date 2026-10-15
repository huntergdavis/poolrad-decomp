#!/usr/bin/env python3
"""Same-run contiguous C49 caller and Close CPU prefix, stopping before DOS."""
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
PLAN=[['0c49', 10894, 3145], ['0c4d', 10894, 3149], ['0c4e', 10894, 3150], ['0c4f', 10894, 3151], ['1491', 4892, 5265], ['1493', 4892, 5267], ['1497', 4892, 5271], ['14b6', 4892, 5302], ['14bc', 4892, 5308], ['14c4', 4892, 5316], ['149a', 4892, 5274], ['149c', 4892, 5276], ['149f', 4892, 5279], ['14a2', 4892, 5282], ['14a4', 4892, 5284], ['14a6', 4892, 5286]]
RAW_HASH='d44c2d96d1145507b450d74a6f54172ca19a76d50d09e42c7621c793147e0744'
RAM_HASHES={'0c49': 'ae54f48945deef576b07897ed13ba5973c677b19cb70fa953f7c265ecd426c7f', '0c4d': 'ae54f48945deef576b07897ed13ba5973c677b19cb70fa953f7c265ecd426c7f', '0c4e': 'ae54f48945deef576b07897ed13ba5973c677b19cb70fa953f7c265ecd426c7f', '0c4f': 'ae54f48945deef576b07897ed13ba5973c677b19cb70fa953f7c265ecd426c7f', '1491': 'd2d00f5bbb915b442babe276efaf09ede9365db573316b10254de4ed313f746a', '1493': 'd2d00f5bbb915b442babe276efaf09ede9365db573316b10254de4ed313f746a', '1497': 'd2d00f5bbb915b442babe276efaf09ede9365db573316b10254de4ed313f746a', '14b6': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '14bc': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '14c4': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '149a': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '149c': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '149f': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '14a2': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '14a4': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075', '14a6': '191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075'}
CALLER_HASH='1da7692556a283d48020cff03bfe4d9749af1f1a495fde4a3d870f4ea78ac67d'
CLOSE_HASH='83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858'
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 assert len(rows)==len(PLAN)==16
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(PLAN,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,h) for n,h in RAM_HASHES.items()}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 body=start[0xAC76+0x1491:0xAC76+0x14C5];assert len(body)==52 and sha256(body).hexdigest()==CLOSE_HASH
 caller=bytearray(game[0x1DFE6+0xC49:0x1DFE6+0xC54])
 assert len(caller)==11 and sha256(caller).hexdigest()==CALLER_HASH and caller[6:]==bytes.fromhex('9a9114f80a')
 caller[9:11]=bytes.fromhex('1c13')
 for n,cs,ip in PLAN:
  assert len(cpus[n])==16 and cpus[n][:2]==[cs,ip]
  assert cpus[n]==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  assert images[n][0x131C0+0x1491:0x131C0+0x14C5]==body,n
  assert images[n][0x2A8E0+0xC49:0x2A8E0+0xC54]==caller,n
 assert images['0c4f'][0x20548:0x2054C]==bytes.fromhex('2636161d')
 assert images['1491'][0x20544:0x2054C]==bytes.fromhex('540c8e2a2636161d')
 assert images['14b6'][0x20542:0x20544]==bytes.fromhex('9a14')
 assert cpus['0c49'][7:10]==[0x33EC,0x37A6,0x11D]
 assert cpus['14a6'][7:11]==[0x33E4,0x37A6,0x3E1D,6] and cpus['14a6'][-1]==0x7202
 a,b=images['0c49'],images['14a6']
 assert {i for i,(x,y) in enumerate(zip(a,b)) if x!=y}<=set(range(0x20542,0x2054C))
 assert a[0x20786:0x20806]==b[0x20786:0x20806]
 assert a[0x20786:0x2078C]==bytes.fromhex('0600b3d70100')
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image))
 state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);native=0;started=time.monotonic()
 for step in range(100):
  if time.monotonic()-started>=10:raise TimeoutError(label)
  pc=(state.cpu.cs,state.cpu.ip);before=cpu_values(state.cpu),memory.raw;r=Report()
  result=lib.pr_slot_a_c49_close_step(ct.byref(state),ct.byref(r))
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
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_c49_close_prefix.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 lib.pr_slot_a_c49_close_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_c49_close_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal=0x14A6,status=1,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 run('native-contiguous-C49-Close',images['0c49'],cpus['0c49'],checkpoints=[(n,cpus[n],images[n]) for n,_,_ in PLAN[1:]])
 def frame(image,off,value):
  a=0x1D160+off;image[a:a+2]=value.to_bytes(2,'little')
 for handle in (0,1,2,3,4,5,6,0x7FFF,0x8000,0xFFFF):
  for flags in (0x7202,0x7602,0x7AD7):
   image=bytearray(images['0c49']);frame(image,0x3626,handle)
   values=cpus['0c49'].copy();values[-1]=flags
   run(f'controlled-caller-handle-{handle:04x}-flags-{flags:04x}',bytes(image),values,0x14AD if handle<=4 else 0x14A6,2 if handle<=4 else 1)
 for ax in (0,0xAB00,0xAB01,0xFFFF):
  for flags in (0x7202,0x7602,0x7AD7):
   values=cpus['0c49'].copy();values[9]=ax;values[-1]=flags
   run(f'controlled-caller-AX-{ax:04x}-flags-{flags:04x}',images['0c49'],values)
 for signature in (0,0xD7B0,0xD7B2,0xD7B4,0xFFFF):
  image=bytearray(images['0c49']);frame(image,0x3628,signature)
  run(f'controlled-caller-signature-{signature:04x}',bytes(image),cpus['0c49'],0x14BE,3)
 for off in (0x33E0,0x33E2,0x33E8):
  image=bytearray(images['0c49']);values=cpus['0c49'].copy();values[8]=(off+0x180)&65535
  frame(image,off,6);frame(image,off+2,0xD7B3)
  run(f'controlled-caller-record-frame-alias-{off:04x}',bytes(image),values,0x14BE,3)
 for off in (0x8000,0xFFFE):
  image=bytearray(images['0c49']);values=cpus['0c49'].copy();values[8]=(off+0x180)&65535
  frame(image,off,0xFFFF);frame(image,(off+2)&65535,0xD7B3)
  run(f'controlled-caller-record-offset-{off:04x}',bytes(image),values)
 for sp in (4,8):
  values=cpus['0c49'].copy();values[7]=sp
  run(f'controlled-caller-far-near-stack-wrap-{sp:04x}',images['0c49'],values)
 values=cpus['149a'].copy();values[-1]&=~0x40
 run('controlled-independent-failed-helper-branch',images['149a'],values,0x14B3,4)
 assert native==15
 print(f'PASS {total} C/Unicorn full-state comparisons;{native} same-run native full checkpoints;16 states;one contiguous C49/far-CALL/Close CPU prefix;no flag masks,DOS effects or CALL-only count',flush=True)
if __name__=='__main__':main()
