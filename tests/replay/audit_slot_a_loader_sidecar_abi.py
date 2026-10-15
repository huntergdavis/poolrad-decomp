#!/usr/bin/env python3
"""Two loader selector contexts; runtime effects remain explicit gaps."""
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
PLANS=[[['0c8f', 10894, 3215], ['0c9b', 10894, 3227], ['c9b-b0b', 4892, 2827], ['0ca0', 10894, 3232], ['0ca5', 10894, 3237], ['ca5-b98', 4892, 2968], ['ca5-bc1', 4892, 3009], ['0caa', 10894, 3242], ['caa-02eb', 4134, 747], ['caa-0300', 4134, 768], ['caa-b25', 4892, 2853], ['caa-0305', 4134, 773], ['caa-0310', 4134, 784]], [['0caf', 10894, 3247], ['0cb1', 10894, 3249], ['0cb6', 10894, 3254], ['0cb9', 10894, 3257], ['0cba', 10894, 3258]], [['0d8f', 10894, 3471], ['0d9b', 10894, 3483], ['d9b-b0b', 4892, 2827], ['0da0', 10894, 3488], ['0da5', 10894, 3493], ['da5-b98', 4892, 2968], ['da5-bc1', 4892, 3009], ['0daa', 10894, 3498], ['daa-02eb', 4134, 747], ['daa-0300', 4134, 768], ['daa-b25', 4892, 2853], ['daa-0305', 4134, 773], ['daa-0310', 4134, 784]], [['0daf', 10894, 3503], ['0db1', 10894, 3505], ['0db6', 10894, 3510], ['0db9', 10894, 3513], ['0dba', 10894, 3514]]]
RAW_HASH='4f2a7c7374b2447d9b0bbb5908698ca4e23b1fb4338a30021801c6a94c68c76a'
RAM_HASHES={'0c8f': '3062710a29e3be5647f908b2526bd3548c4d9669ebb39d52d89d8a26b8472145', '0c9b': 'e309501548a0071be1f5b0c5dcd0c78c3228ef7f157a58ed3f32813889ec6d4a', 'c9b-b0b': '4b9324dc7be0f542292dfdd7d87f5df23cec3ba88fc2ffc1afb59fb26babdcf5', '0ca0': '6b08aa5f3686d1548e4ecd8876ea949c5ca3f3e86ee413b755ee42cdbaeacab1', '0ca5': '53dc7023dbe3f0805946c8665c9ce348cbedf14c9f92d37ed8f5304c3aa1ac35', 'ca5-b98': 'ca69de63bff7bd51513c23faf79490fed74c3d491e106aa52d66606b72c0dd21', 'ca5-bc1': '58e352b30b66d989250fc46ea5436b2dc10b80939a6caa94e2b4d1240163cbc4', '0caa': '58e352b30b66d989250fc46ea5436b2dc10b80939a6caa94e2b4d1240163cbc4', 'caa-02eb': 'b6f5aeecc31b26b9e955cbea763693ddaaa09ffb272676ecc64e68eab86d1222', 'caa-0300': '751da9b7fde1028f6a074ec767b0c7555c70ceb7027aa6018d92f5c0ee785327', 'caa-b25': 'e919b14c5117d3d86059414f8f3da738562bb5fd832a68987cd26759db02179e', 'caa-0305': '7d50e74fa839e5aba9c84b312048ad0a366ca9056840fbc3acf66f894f96a347', 'caa-0310': 'de5a2600bd754ec1990592a7b4821cb070ee44155cb03f3b798a75af07a1dd6b', '0caf': '25dc8ce57f3a9343689635b34017bed10f2fd1b83e5dc54fcc192cb94c1f13d4', '0cb1': '25dc8ce57f3a9343689635b34017bed10f2fd1b83e5dc54fcc192cb94c1f13d4', '0cb6': '25dc8ce57f3a9343689635b34017bed10f2fd1b83e5dc54fcc192cb94c1f13d4', '0cb9': '25dc8ce57f3a9343689635b34017bed10f2fd1b83e5dc54fcc192cb94c1f13d4', '0cba': '6c130a09faaff8c6398c87918e511a95fa97e8111895b4066dc9be1acfb485cb', '0d8f': 'ae19b0f97c827ba6aaf1471d23243eaf5638f1fb28ee4a6e39fbd532a7cd64bb', '0d9b': '9894f68bd0c5c78ac18b110bc12aaf86a4a7e1bf254ba5cfd21f9ebd67c89523', 'd9b-b0b': '84b3b243117f0d6ca94b1f1673deb515c0182a259162a13b215951dea8b8eb20', '0da0': '3f7364ea93784ba4978f4f94b066611733aa6642590890ec1cdc5b638a81a327', '0da5': '23d5d0ea7d3584e126c851d9422b3ecb6790de46e638c37253974d22ee415dc2', 'da5-b98': 'ab509e3b3957b648b3cc7a802c0e7d6d3096e6be719ba378a2c85f4b5a4e219f', 'da5-bc1': 'f02084ae910f6e1c8d539d9b14eb1e9b399b1d2770e21e9573b25c499839d53c', '0daa': 'f02084ae910f6e1c8d539d9b14eb1e9b399b1d2770e21e9573b25c499839d53c', 'daa-02eb': '7b9d0dc867db80ab7529a4595f6746a34f649cce82523d4aab9b962058875a3d', 'daa-0300': 'bb7104ca6dec7376190a74ff96b5447483e80fc561e11eda20dea44233e52604', 'daa-b25': '7be546a5f2a6738d9d842e25a4f3b9e20da87cca146fa52e078765f1dd83391d', 'daa-0305': '0ade928c9bc5c785f2538871f00e37ec52741013ee52b85ac9d547b3f81bd524', 'daa-0310': '5f180423bc84d48a773fcd97bf6323cef6c793b4c870c806caafcd4d9b4180e6', '0daf': '1498142e29fa70d1a472ecb72935451756159db7136a7e1935b13b0a9e1cf231', '0db1': '1498142e29fa70d1a472ecb72935451756159db7136a7e1935b13b0a9e1cf231', '0db6': '1498142e29fa70d1a472ecb72935451756159db7136a7e1935b13b0a9e1cf231', '0db9': '1498142e29fa70d1a472ecb72935451756159db7136a7e1935b13b0a9e1cf231', '0dba': 'd08d3c2ba7c3cdc0524e3b7b763cdee130576d61693b3e63d9c5ccc73d0e96b8'}
def inputs(root):
 raw=(root/'native.log').read_bytes();assert sha256(raw).hexdigest()==RAW_HASH
 clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
 rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
 stops=[stop for plan in PLANS for stop in plan];assert len(rows)==len(stops)==36
 cpus={n:[int(v,16) for v in row.split()] for (n,_,_),row in zip(stops,rows)}
 images={n:mapped.pins.checked(root/f'ram-{n}.bin',655360,RAM_HASHES[n]) for n,_,_ in stops}
 start=mapped.pins.checked(root/'POOLRAD/START.EXE',65231,mapped.pins.START_HASH)
 game=mapped.pins.checked(root/'POOLRAD/GAME.OVR',206105,mapped.pins.GAME_HASH)
 mapped.pins.checked(root/'POOLRAD/SAVGAMA.DAT',13137,mapped.pins.SAVE_HASH)
 bodies=[]
 for pc,size,digest in [(0xB0B,26,'4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f'),(0xB98,44,'c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64'),(0xB25,36,mapped.pins.COPY_HASH)]:
  body=start[0xAC76+pc:0xAC76+pc+size];assert sha256(body).hexdigest()==digest;bodies.append((0x131C,pc,body))
 for pc,size,digest,calls in [(0xC8F,32,'a90b1d46e356c4fbe0553e89da3637e1da7697c98f8d7ad53c5c332cb55b14b1',(0xC9B,0xCA5,0xCAA)),(0xD8F,32,'d10e9abf5707ebeaa3277d66e3707cc7f8b49d747a6a1ed3f37d98ee6ef2282f',(0xD9B,0xDA5,0xDAA)),(0xCAF,16,'6a6980b142121eccd959ff3ea69319c89a5a3e843ab69af6f26cc16721cffae1',(0xCBA,)),(0xDAF,16,'ac34b4c897a9bbd92fa302c56f8c06cf0b501d3591b457ec1f5b384596abdc7c',(0xDBA,)),(0xE8D,5,'ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356',(0xE8D,))]:
  original=game[0x1DFE6+pc:0x1DFE6+pc+size];assert sha256(original).hexdigest()==digest;loaded=bytearray(original)
  for call in calls:
   i=call-pc;assert loaded[i]==0x9A;seg=int.from_bytes(loaded[i+3:i+5],'little');assert seg in (0xAF8,0x802,0x899)
   loaded[i+3:i+5]=(seg+0x824).to_bytes(2,'little')
  bodies.append((0x2A8E,pc,bytes(loaded)))
 original=start[0x8342:0x83AD]
 assert sha256(original).hexdigest()=='141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474'
 loaded=bytearray(original)
 for call in (0x300,0x310,0x31F,0x32A,0x32F):
  i=call-0x2EB;assert loaded[i]==0x9A and loaded[i+3:i+5]==(0xAF8).to_bytes(2,'little');loaded[i+3:i+5]=(0x131C).to_bytes(2,'little')
 bodies.append((0x1026,0x2EB,bytes(loaded)))
 for n,cs,ip in stops:
  values=cpus[n];assert len(values)==16 and values[:2]==[cs,ip]
  assert values==[json.loads((root/f'cpu-{n}.json').read_text())[f] for f in NAMES]
  for seg,pc,body in bodies:assert images[n][(seg<<4)+pc:(seg<<4)+pc+len(body)]==body,(n,hex(pc))
 def words(n,off,count):
  b=images[n];return [int.from_bytes(b[0x1D160+off+2*i:0x1D160+off+2*i+2],'little') for i in range(count)]
 for prefix in ('c','d'):
  assert words(prefix+'9b-b0b',0x33E0,6)==[int(prefix+'a0',16),0x2A8E,0x36A6,0x1D16,0x34ED,0x1D16]
  assert words(prefix+'a5-b98',0x33E0,6)==[int(prefix+'aa',16),0x2A8E,0xAE2 if prefix=='c' else 0xAE7,0x2A8E,0x34ED,0x1D16]
  assert words('0'+prefix+'aa',0x33E8,2)==[0x34ED,0x1D16]
  assert words(prefix+'aa-02eb',0x33E4,4)==[int(prefix+'af',16),0x2A8E,0x34ED,0x1D16]
  assert images['0'+prefix+'8f'][0x20806]==19
  assert images['0'+prefix+'a0'][0x2064D]==19 and images['0'+prefix+'aa'][0x2064D]==23
  assert images[prefix+'aa-0305'][0x20518]==23
  assert cpus['0'+prefix+'af'][9]==1
  assert images['0'+prefix+'af'][0x193CE:0x193D0]==bytes(2)
 return cpus,images
def differential(lib,label,image,values,return_fragment,terminal,status,checkpoints=()):
 memory=ct.create_string_buffer(image,len(image));state=State(CPU(**dict(zip(NAMES,values))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
 uc=mapped.emulator(image,values);pending=list(checkpoints);seen=own=0;started=time.monotonic()
 fn=lib.pr_slot_a_loader_sidecar_return_step if return_fragment else lib.pr_slot_a_loader_sidecar_prefix_step
 for step in range(200):
  pc=(state.cpu.cs,state.cpu.ip)
  if time.monotonic()-started>=20:raise TimeoutError(f'{label}: deadline at {pc}')
  before_cpu,before_ram=cpu_values(state.cpu),memory.raw;r=Report();result=fn(ct.byref(state),ct.byref(r))
  if result:
   assert pc==terminal and result==status and not pending,(label,pc,result)
   assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
   fields=(r.dependency_valid,r.caller,r.call_pc,r.callee_segment,r.callee_pc,r.return_pc,r.caller_is_overlay)
   if status==1:assert fields==(1,0x802,0x310,0xAF8,0x13E2,0x315,0)
   elif status==2:assert fields==(1,0x6F,pc[1],0xAF8,0x23F,pc[1]+5,1)
   elif status==3:assert fields==(1,0x6F,0xE8D,0x899,0x104,0xE92,1)
   else:assert not r.dependency_valid
   print(f'{label}: {step} full-state comparisons ({own} new); {seen} native checkpoints; typed stop {status}',flush=True)
   return step,own,seen
  assert not r.dependency_valid;mapped.one(uc)
  compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'{label} after {pc}')
  assert memory.raw==bytes(uc.mem_read(0,len(image))),(label,pc,'RAM')
  own+=pc[0]==0x2A8E
  if pending and cpu_values(state.cpu)[:2]==pending[0][1][:2]:
   n,expected_cpu,expected_ram=pending.pop(0)
   compare_cpu(cpu_values(state.cpu),expected_cpu,n);assert memory.raw==expected_ram,n;seen+=1
 raise AssertionError(f'{label}: instruction bound')
def main():
 if len(sys.argv)!=3:raise SystemExit('usage: audit_slot_a_loader_sidecar_abi.py LIBRARY NATIVE_ROOT')
 if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn2.1.4 and assertions')
 library,root=(Path(p).resolve(strict=True) for p in sys.argv[1:])
 if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
  raise SystemExit('library and inputs must stay in this clone resources/')
 cpus,images=inputs(root);lib=ct.CDLL(str(library))
 for name in ('prefix','return'):
  fn=getattr(lib,'pr_slot_a_loader_sidecar_'+name+'_step');fn.argtypes=[ct.POINTER(State),ct.POINTER(Report)];fn.restype=ct.c_int
 total=own=native=0
 def run(label,image,values,returned=False,terminal=(0x1026,0x310),status=1,checkpoints=()):
  nonlocal total,own,native
  n,o,v=differential(lib,label,image,values,returned,terminal,status,checkpoints);total+=n;own+=o;native+=v
 for i,plan in enumerate(PLANS):
  n=plan[0][0];returned=bool(i%2);terminal=(0x2A8E,0xCBA if i==1 else 0xDBA) if returned else (0x1026,0x310)
  run(f'native-fragment-{n}',images[n],cpus[n],returned,terminal,2 if returned else 1,[(label,cpus[label],images[label]) for label,_,_ in plan[1:]])
 # Every controlled input is a fresh copy; no runtime result updates a live C run.
 for site in ('0c8f','0d8f'):
  for length in (0,1,40,255):
   image=bytearray(images[site]);image[0x20806]=length
   image[0x20807:0x20807+length]=bytes(65+i%26 for i in range(length))
   values=cpus[site].copy();values[-1]|=0x400
   run(f'controlled-{site}-length-{length}-initial-DF',bytes(image),values)
 for label,offset,seg in [('identical',0x34ED,0x1D16),('physical-alias',0x34DD,0x1D17),('forward-overlap',0x34EC,0x1D16)]:
  image=bytearray(images['c9b-b0b']);image[0x20544:0x20548]=offset.to_bytes(2,'little')+seg.to_bytes(2,'little')
  if label=='forward-overlap':image[0x2064C:0x20650]=bytes([3,65,66,67])
  run(f'controlled-B0B-{label}',bytes(image),cpus['c9b-b0b'])
 for site in ('0caf','0daf'):
  for ax in (0,1,0x100,0x101,0x8000,0x8001,0xFF00,0xFFFF):
   values=cpus[site].copy();values[9]=ax;values[-1]=0x7AD7
   if ax&255:terminal,status=(0x2A8E,0xCBA if site=='0caf' else 0xDBA),2
   else:terminal,status=(0x2A8E,0xD8F if site=='0caf' else 0xE8D),(4 if site=='0caf' else 3)
   run(f'controlled-{site}-AX-{ax:04x}',images[site],values,True,terminal,status)
 assert native==32
 print(f'PASS {total} complete CPU/RAM comparisons ({own} new, {total-own} unchanged helper); {native} native checkpoints;36 native states; four independent fragments; runtime effects excluded',flush=True)
if __name__=='__main__':main()
