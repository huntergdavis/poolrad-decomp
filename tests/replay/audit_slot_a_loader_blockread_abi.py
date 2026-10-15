#!/usr/bin/env python3
"""Loader read/Close CPU fragments with explicit runtime and DOS gaps."""
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
PLANS=[[['0c2b', 10894, 3115], ['0c2f', 10894, 3119], ['0c33', 10894, 3123], ['0c34', 10894, 3124], ['0c35', 10894, 3125], ['0c38', 10894, 3128], ['0c39', 10894, 3129], ['0c3a', 10894, 3130], ['0c3d', 10894, 3133], ['0c3e', 10894, 3134], ['0c42', 10894, 3138], ['0c43', 10894, 3139], ['0c44', 10894, 3140]], [['0c49', 10894, 3145], ['0c4d', 10894, 3149], ['0c4e', 10894, 3150], ['0c4f', 10894, 3151]], [['0c54', 10894, 3156], ['0c57', 10894, 3159], ['0c59', 10894, 3161], ['0c5e', 10894, 3166], ['0c63', 10894, 3171], ['0c66', 10894, 3174], ['0c68', 10894, 3176], ['0c6d', 10894, 3181], ['0c72', 10894, 3186], ['0c75', 10894, 3189], ['0c77', 10894, 3191], ['0c7b', 10894, 3195], ['0c80', 10894, 3200], ['0c83', 10894, 3203], ['0c85', 10894, 3205], ['0c8a', 10894, 3210], ['0c8f', 10894, 3215]]]
STOPS=[['0c2b', 10894, 3115], ['0c2f', 10894, 3119], ['0c33', 10894, 3123], ['0c34', 10894, 3124], ['0c35', 10894, 3125], ['0c38', 10894, 3128], ['0c39', 10894, 3129], ['0c3a', 10894, 3130], ['0c3d', 10894, 3133], ['0c3e', 10894, 3134], ['0c42', 10894, 3138], ['0c43', 10894, 3139], ['0c44', 10894, 3140], ['read-entry', 4892, 5371], ['read-dos-input', 4892, 5417], ['read-dos-return', 4892, 5419], ['0c49', 10894, 3145], ['0c4d', 10894, 3149], ['0c4e', 10894, 3150], ['0c4f', 10894, 3151], ['close-entry', 4892, 5265], ['0c54', 10894, 3156], ['0c57', 10894, 3159], ['0c59', 10894, 3161], ['0c5e', 10894, 3166], ['0c63', 10894, 3171], ['0c66', 10894, 3174], ['0c68', 10894, 3176], ['0c6d', 10894, 3181], ['0c72', 10894, 3186], ['0c75', 10894, 3189], ['0c77', 10894, 3191], ['0c7b', 10894, 3195], ['0c80', 10894, 3200], ['0c83', 10894, 3203], ['0c85', 10894, 3205], ['0c8a', 10894, 3210], ['0c8f', 10894, 3215]]
RAW_HASH='e5bc8ffffb93343bc6fa0f9f7846d7459a223638e1fee1ac274bf0bff77966a5'
RAM_HASHES={'0c2b': '7d8f9db2f0502d9ec3864e1c310fe80e05c5465654307109a07c793aff53d323', '0c2f': 'c0733b0822f4edc2c6e6333cb61990dd42c1c31637108a1b4982a1f3c7ed7d5a', '0c33': 'c0733b0822f4edc2c6e6333cb61990dd42c1c31637108a1b4982a1f3c7ed7d5a', '0c34': 'c0733b0822f4edc2c6e6333cb61990dd42c1c31637108a1b4982a1f3c7ed7d5a', '0c35': '4356cde83010fe402d61da7ec8db0d61307130372158e65943469f88762984d8', '0c38': '4356cde83010fe402d61da7ec8db0d61307130372158e65943469f88762984d8', '0c39': '9ee123f111e4b181d5dee32b812c901da580926fcd07f48dfab24e72588284e0', '0c3a': '9de49e591d45157e5f2d9911e79c6238b10cb71ada91b1593298f5f1c838821b', '0c3d': '9de49e591d45157e5f2d9911e79c6238b10cb71ada91b1593298f5f1c838821b', '0c3e': 'eeed02d9ad897797984e155a620df0dc78b2810ab1ce22a21027c04c1124028e', '0c42': 'eeed02d9ad897797984e155a620df0dc78b2810ab1ce22a21027c04c1124028e', '0c43': 'eeed02d9ad897797984e155a620df0dc78b2810ab1ce22a21027c04c1124028e', '0c44': '9184c464e553de4d39457ae45db78c4bfc14e22e9d946ce8f7ddf80a9eca2bef', 'read-entry': '3deb00d8f0a994b9ee87fc9a4ed118a8308d06b31756fdaee706c42bb8e241f9', 'read-dos-input': '3e5079ce90b5adb9d673ac67812af4c1d8cf79e59638e791ce8828f89e2ccd3c', 'read-dos-return': '24a0b13c72ec11e7f537eab068441b2b16f0d41f7baefa22ec48a9eace0fd6b4', '0c49': 'e789f82d20a23008d07f2223b16bebe13cc7f426950cd0c80cdc620053247a59', '0c4d': 'e789f82d20a23008d07f2223b16bebe13cc7f426950cd0c80cdc620053247a59', '0c4e': 'e789f82d20a23008d07f2223b16bebe13cc7f426950cd0c80cdc620053247a59', '0c4f': 'e789f82d20a23008d07f2223b16bebe13cc7f426950cd0c80cdc620053247a59', 'close-entry': '530dfa809b1a32652888844cd16ea7592d15510e796b21883bb76a250213f774', '0c54': 'd5af1b0acf6892d753d2d5e063660b8a03fe1cdeb3ebd32d2741514e941e7e91', '0c57': 'd5af1b0acf6892d753d2d5e063660b8a03fe1cdeb3ebd32d2741514e941e7e91', '0c59': 'd5af1b0acf6892d753d2d5e063660b8a03fe1cdeb3ebd32d2741514e941e7e91', '0c5e': '952ea582f73de3e4d1a8bf1643cdfaf7fc173a0a417408d5a604cbe648aaf871', '0c63': '22cb4907c0fb82a31fc77ce7d146e3fee906072309dd25d430d5b61fb147a559', '0c66': '22cb4907c0fb82a31fc77ce7d146e3fee906072309dd25d430d5b61fb147a559', '0c68': '22cb4907c0fb82a31fc77ce7d146e3fee906072309dd25d430d5b61fb147a559', '0c6d': '22cb4907c0fb82a31fc77ce7d146e3fee906072309dd25d430d5b61fb147a559', '0c72': '0680659716c4830ca4bbb3f08a681f212736ba9505f9b1d0a28ee1e4353e4bc0', '0c75': '0680659716c4830ca4bbb3f08a681f212736ba9505f9b1d0a28ee1e4353e4bc0', '0c77': '0680659716c4830ca4bbb3f08a681f212736ba9505f9b1d0a28ee1e4353e4bc0', '0c7b': '1196c981343d3378256c31eb00b3a4551111d985d348e16600fefcad20b461fb', '0c80': 'df359ad6e4851c5cda09805382bcf02373ead8105fb375940b1b95c99a16fdaa', '0c83': 'df359ad6e4851c5cda09805382bcf02373ead8105fb375940b1b95c99a16fdaa', '0c85': 'df359ad6e4851c5cda09805382bcf02373ead8105fb375940b1b95c99a16fdaa', '0c8a': 'df359ad6e4851c5cda09805382bcf02373ead8105fb375940b1b95c99a16fdaa', '0c8f': 'df359ad6e4851c5cda09805382bcf02373ead8105fb375940b1b95c99a16fdaa'}
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean);assert len(rows)==len(STOPS)==38
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(STOPS,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,RAM_HASHES[n]) for n,_,_ in STOPS}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 original=game[0x1DFE6+0xC2B:0x1DFE6+0xC8F]
 assert len(original)==100 and sha256(original).hexdigest()=='72ce2055c7a265398578b903043b5b924402632b06c457c48c915f085122f529'
 loaded=bytearray(original)
 for call,callee in ((0xC44,0x14FB),(0xC4F,0x1491)):
  i=call-0xC2B;assert loaded[i:i+5]==bytes([0x9A])+callee.to_bytes(2,'little')+(0xAF8).to_bytes(2,'little');loaded[i+3:i+5]=(0x131C).to_bytes(2,'little')
 bodies=[(0x2A8E,0xC2B,bytes(loaded))]
 for pc,n,digest in [(0x14FB,32,'ff0660891ecb3fd6c09cb7a346f640f29b7b5f294ecfd918cb90e07b02fb1b16'),(0x1491,52,'83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858')]:
  b=start[0xAC76+pc:0xAC76+pc+n];assert sha256(b).hexdigest()==digest;bodies.append((0x131C,pc,b))
 assert start[0xAC76+0x1529:0xAC76+0x152B]==bytes.fromhex('cd21');bodies.append((0x131C,0x1529,bytes.fromhex('cd21')))
 for n,cs,ip in STOPS:
  values=cpus[n];assert len(values)==16 and values[:2]==[cs,ip]
  assert values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  for seg,pc,b in bodies:assert images[n][(seg<<4)+pc:(seg<<4)+pc+len(b)]==b,(n,hex(pc))
 def words(n,off,count):
  b=images[n];return [int.from_bytes(b[0x1D160+off+2*i:0x1D160+off+2*i+2],'little') for i in range(count)]
 assert words('0c44',0x33DE,7)==[0x3624,0x1D16,0x11D,8,0x449A,0x3626,0x1D16]
 assert words('read-entry',0x33DA,9)==[0xC49,0x2A8E,0x3624,0x1D16,0x11D,8,0x449A,0x3626,0x1D16]
 assert words('close-entry',0x33E4,4)==[0xC54,0x2A8E,0x3626,0x1D16]
 a,b=(cpus[n] for n in ('read-dos-input','read-dos-return'))
 assert (a[9],a[10],a[11],a[2],a[12])==(0x3F1D,6,0x11D,0x449A,8)
 assert b[9]==0x11D and not b[-1]&1 and all(a[i]==b[i] for i in range(16) if i not in (1,9))
 a,b=(images[n] for n in ('read-dos-input','read-dos-return'))
 assert sum(x!=y for x,y in zip(a,b))==145
 assert sum(x!=y for x,y in zip(a[0x449A8:0x44AC5],b[0x449A8:0x44AC5]))==126
 assert words('0c49',0x3624,1)==[0x11D]
 a,b=(images[n] for n in ('0c54','0c8f'))
 allowed={0x449A8+o+i for o in (0x104,0x106,0xC8,0xCA,0x7F,0x81,0x108,0x10A) for i in (0,1)}
 changed={i for i,(x,y) in enumerate(zip(a,b)) if x!=y};assert len(changed)==8 and changed<=allowed
 assert all(b[i]==0 for i in allowed)
 assert cpus['0c8f'][9]==0 and cpus['0c8f'][-1]==0x7246
 return cpus,images
def differential(lib,label,image,values,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image));state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);seen=0;started=time.monotonic()
 for step in range(100):
  pc=(state.cpu.cs,state.cpu.ip)
  if time.monotonic()-started>=10:raise TimeoutError(f'{label}: deadline at {pc}')
  before_cpu,before_ram=cpu_values(state.cpu),memory.raw;r=Report();result=lib.pr_slot_a_loader_blockread_step(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
   if status in (1,2):
    assert (r.dependency_valid,r.caller,r.call_pc,r.callee_segment,r.callee_pc,r.return_pc,r.caller_is_overlay)==(1,0x6F,pc[1],0xAF8,0x14FB if status==1 else 0x1491,pc[1]+5,1)
   else:assert not r.dependency_valid
   print(f'{label}: {step} C full-state comparisons; {seen} native checkpoints; typed stop {status}',flush=True)
   return step,seen
  assert not r.dependency_valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}');assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  if pending:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;seen+=1
 raise AssertionError(f'{label}: instruction bound')

def native_call_frames(cpus,images):
 for before,after in [('0c44','read-entry'),('0c4f','close-entry')]:
  uc=mapped.emulator(images[before],cpus[before]);mapped.one(uc)
  compare_cpu(mapped.values(uc),cpus[after],f'original CALL-only {before}');assert bytes(uc.mem_read(0,655360))==images[after]
 print('PASS two original-CALL-only frame comparisons; runtime/DOS effects excluded from C',flush=True)
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_loader_blockread_abi.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);native_call_frames(cpus,images);lib=ct.CDLL(str(library))
 lib.pr_slot_a_loader_blockread_step.argtypes=[ct.POINTER(State),ct.POINTER(Report)];lib.pr_slot_a_loader_blockread_step.restype=ct.c_int
 total=native=0
 def run(label,image,values,terminal,status,checkpoints=()):
  nonlocal total,native
  n,v=differential(lib,label,image,values,terminal,status,checkpoints);total+=n;native+=v
 for plan,status in zip(PLANS,(1,2,3)):
  first=plan[0][0];last=plan[-1];run(f'native-{first}',images[first],cpus[first],(last[1],last[2]),status,[(n,cpus[n],images[n]) for n,_,_ in plan[1:]])
 # Initial copies only. No native read/Close/DOS result replaces a live C state.
 for ax in (0,1,0xAB00,0xAB01,0xFFFF):
  for flags in (0x7202,0x7602,0x7AD7):
   values=cpus['0c2b'].copy();values[9]=ax;values[-1]=flags
   run(f'controlled-setup-AX-{ax:04x}-flags-{flags:04x}',images['0c2b'],values,(0x2A8E,0xC44),1)
 for label,seg,off in [('native-address',0x449A,8),('alternate-address',0x3000,0x2000),('offset-wrap',0x2000,0xFF00),('argument-storage-alias',0x1D16,0x36AC)]:
  image=bytearray(images['0c54'])
  for displacement in (0x104,0x106,0xC8,0xCA,0x7F,0x81,0x108,0x10A):
   p=(seg<<4)+((off+displacement)&65535);image[p:p+2]=bytes([0xA5,0xA5])
  image[0x20910:0x20914]=off.to_bytes(2,'little')+seg.to_bytes(2,'little')
  values=cpus['0c54'].copy();values[9]=0xFFFF;values[-1]=0x7AD7
  run(f'controlled-clears-{label}',bytes(image),values,(0x2A8E,0xC8F),3)
 values=cpus['0c59'].copy();values[9]=0xABCD
 run('controlled-independent-word-store-AX',images['0c59'],values,(0x2A8E,0xC8F),3)
 for ax in (0,0xFFFF):
  values=cpus['0c49'].copy();values[9]=ax
  run(f'controlled-independent-Close-setup-{ax:04x}',images['0c49'],values,(0x2A8E,0xC4F),2)
 image=bytearray(images['0c2b']);image[0x20910:0x20914]=bytes.fromhex('00800020')
 run('controlled-alternate-read-buffer-pointer',bytes(image),cpus['0c2b'],(0x2A8E,0xC44),1)
 assert native==31
 print(f'PASS {total} C full-state comparisons; {native} native C checkpoints;38 states;2 separate original-CALL frame checks;2 native DOS states excluded; three independent fragments',flush=True)
if __name__=='__main__':main()
