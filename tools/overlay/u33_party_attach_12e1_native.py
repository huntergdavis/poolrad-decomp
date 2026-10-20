#!/usr/bin/env python3
"""U33 +12E1 DOSBox-X normal/386 synthetic native corroboration.

Completed calls preserve all original body bytes except loader far segments.
RunError and requested instruction-budget boundaries use explicit far-jump
interceptions; their CS:IP coordinates are declared, not captured registers.
All cases compare fourteen CPU words without masks and five full64KiB data
regions. Completed calls capture13 direct words+known return IP; interceptions
capture12 direct words+2 declared coordinates. No naturally reached route claim.
"""
import argparse,json,shutil,struct
from hashlib import sha256
from pathlib import Path
from u33_party_attach_12e1_audit import ROOT,Cpu,physical,put16,fixture,source,install,build_model,run_c,original
from u9e_character_pair_115c_native import assemble,assembly,launch,find_layout
OUT=ROOT/'resources/u33-native'


def native_case(directory,base,options,bodies,decoded,lib,stop_at=None,diagnose=False):
    segments=[base+0x1000,base+0x2000,base+0x3000,base+0x4000,base+0x5000,base+0x6000]
    code,ds,ss,record,extra,heap=segments;resident=code+0x400;system=code+0x200
    opts=dict(cs=code,resident_cs=resident,system_cs=system,ds=ds,ss=ss,record_seg=record,extra_seg=extra,heap_seg=heap);opts.update(options)
    state=fixture(**opts);memory,cpu,_,_=state;install(*state,bodies)
    budget=1000000;checkpoint=None
    if stop_at:
        raw=original(state,decoded,budget,stop_at=stop_at)
        assert raw[0]==2 and (raw[2].cs,raw[2].ip)==(code if stop_at[0]=='u33' else system,stop_at[1])
        budget-=raw[3]
    expected=run_c(lib,state,budget)
    if expected[0]!=0:
        assert (expected[0]==2 and stop_at) or (expected[0]==3 and expected[4] in (2,3)),('unapproved native stop',expected[0],expected[4])
        checkpoint=(expected[2].cs,expected[2].ip)
        raw=original(state,decoded,budget)
        assert (raw[2].cs,raw[2].ip)==checkpoint
        key=('u33' if checkpoint[0]==code else 'res' if checkpoint[0]==resident else 'sys',checkpoint[1])
        assert key not in raw[5],'interception would stop at an earlier repeated visit'
    symbols=assemble(directory,assembly(cpu,segments,checkpoint))
    put16(memory,ss,cpu.sp,symbols['returned']);put16(memory,ss,cpu.sp+2,base)
    expected=run_c(lib,state,budget);raw=original(state,decoded,budget)
    assert expected[1]==raw[1],('raw RAM differs',options)
    differences={name:dict(model=getattr(expected[2],name),unicorn=getattr(raw[2],name)) for name,_ in Cpu._fields_ if getattr(expected[2],name)!=getattr(raw[2],name)}
    native=bytearray(memory)
    if checkpoint:
        at=physical(*checkpoint);native[at:at+5]=b'\xea'+symbols['returned'].to_bytes(2,'little')+base.to_bytes(2,'little')
    for index,segment in enumerate(segments):
        at=physical(segment,0);(directory/f'I{index}.BIN').write_bytes(native[at:at+65536])
    (directory/'RESULT.BIN').unlink(missing_ok=True);launch(directory)
    result=(directory/'RESULT.BIN').read_bytes();assert len(result)==28+5*65536,len(result)
    native_differences={name:dict(model=getattr(expected[2],name),native=value) for (name,_),value in zip(Cpu._fields_,struct.unpack_from('<14H',result)) if getattr(expected[2],name)!=value}
    if not diagnose:assert not native_differences,(options,stop_at,native_differences)
    for index,segment in enumerate(segments[1:]):
        at=physical(segment,0);wanted=expected[1][at:at+65536];got=result[28+index*65536:28+(index+1)*65536]
        assert wanted==got,(options,stop_at,'RAM',hex(segment),[(hex(i),a,b) for i,(a,b) in enumerate(zip(wanted,got)) if a!=b][:12])
    return dict(options=options,budget=budget,checkpoint=list(checkpoint) if checkpoint else None,status=expected[0],getmem_status=expected[4],
                original_entry_complete=checkpoint is None,synthetic_interception=checkpoint is not None,cpu_words_compared=14,
                direct_register_words=12 if checkpoint else 13,known_control_coordinates=2 if checkpoint else 1,
                coordinate_provenance='declared first unexecuted original CS:IP' if checkpoint else 'executed return-label IP',
                ram_bytes=327680,masked_flags=0,raw_unicorn_cpu_differences=differences,native_cpu_mismatches=native_differences,result_sha256=sha256(result).hexdigest())


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);p.add_argument('--quick',action='store_true');p.add_argument('--diagnose',action='store_true');p.add_argument('--dosbox-source',type=Path);args=p.parse_args()
    source_hash='7cbc4bea7e47678f0a3734a4c16c2110ef8125b09d0aa7f7a8457aab7b3f747e'
    if args.dosbox_source:assert sha256((args.dosbox_source/'src/cpu/instructions.h').read_bytes()).hexdigest()==source_hash,'IDIV profile source changed'
    out=args.output_dir;out.mkdir(parents=True,exist_ok=True);base=find_layout(out);bodies,decoded,pins=source();lib=build_model(out)
    specs=[('empty',dict(party=()),None),('top',dict(),None),('skip-attach',dict(threshold=1),None),
           ('tag102',dict(party=({'tag':102},)),None),('tag103',dict(party=({'tag':103},),window_value=255),None),
           ('mode1',dict(party=({'mode':1},),table_value=255,index=255),None),('mode2',dict(party=({'mode':2},)),None)]
    specs += [(mode,dict(heap_mode=mode),None) for mode in ('exact','split','split-carry','skip','top-exact','fail','top-granule-fail')]
    specs += [(f'idiv-{value}',dict(party=({'mode':1},),table_value=value),('u33',0x1395)) for value in ((0,1,2,3,4,5,6,12,20,127,128,251,252,255) if args.quick else range(256))]
    specs += [(f'shr-{index}',dict(index=index),('u33',0x1361)) for index in (0,1,2,3,127,128,254,255)]
    specs += [('failure-after-first',dict(party=({},{}),heap_mode='top-exact'),None),('alias-ds-ss',dict(alias_stack_data=True),None)]
    specs += [(f'custom-{mode}',dict(heap_mode=mode,custom_heap=True),None) for mode in ('top','fail')]
    if not args.quick:
        specs += [(f'count-{count}',dict(party=tuple(dict(mode=i%3,tag=(i*17)&255) for i in range(count)),threshold=2),None) for count in (3,255,256,257)]
        specs += [(f'sp-{sp:x}',dict(sp=sp),None) for sp in (0x100,0x101,0xffef,0xfff0)]
    cases=[]
    for index,(name,options,stop_at) in enumerate(specs):
        directory=out/f'case-{index:02d}-{name}';directory.mkdir(exist_ok=True)
        cases.append(native_case(directory,base,options,bodies,decoded,lib,stop_at,args.diagnose));print(f'{"FAIL" if cases[-1]["native_cpu_mismatches"] else "PASS"} native {index} {name}',flush=True)
    report=dict(target='OVL_0033_12E1',idiv_source=dict(location='src/cpu/instructions.h:743 IDIVW',sha256=source_hash,local_copy_verified=args.dosbox_source is not None),profile='DOSBox-X normal/386',dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(),
                cases=cases,native_cases=len(cases),failed_cases=sum(bool(c['native_cpu_mismatches']) for c in cases),complete_entries=sum(c['original_entry_complete'] for c in cases),
                runtime_tail_interceptions=sum(c['getmem_status']==2 for c in cases),custom_heap_error_interceptions=sum(c['getmem_status']==3 for c in cases),instruction_checkpoint_interceptions=sum(c['status']==2 for c in cases),
                cpu_words_compared=14,ram_bytes_per_case=327680,masked_flags=0,natural_game_route_measured=False)
    (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True));assert report['failed_cases']==0,'native CPU differences remain'
if __name__=='__main__':main()
