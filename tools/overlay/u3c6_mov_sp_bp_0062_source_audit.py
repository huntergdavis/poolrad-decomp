#!/usr/bin/env python3
"""Original/constructed-loaded instruction byte identity; no guest execution."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

ENTRY=0x0062
SOURCE_SHA="59aa786538280cb6e5de5d8b80771d9d5c21914f6dd579661adbe8b5b508f278"
EXE_SHA="d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA="872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"

def check(ok,message):
    if not ok:raise ValueError(message)

# Adapted from reviewed 4ef6fb6 u3c2_jl_decision_0591_static_audit.py.
def verify_loaded_image(image,cs,entry,code):
    check(len(image)==1048576,"loaded image must contain exactly1MiB")
    check(all(type(v) is int and 0<=v<=65535 for v in (cs,entry)),"CS/IP must be explicit16-bit integers")
    check(1<=len(code)<=15 and entry+len(code)<=65536,"instruction crosses approved loaded domain")
    addresses=[(cs*16+entry+i)%1048576 for i in range(len(code))]
    actual=bytes(image[a] for a in addresses)
    check(actual==code,"loaded instruction bytes differ from original source")
    return dict(loaded_cs=cs,loaded_ip=entry,byte_addresses=addresses,
                loaded_instruction_sha256=sha256(actual).hexdigest(),
                loaded_image_sha256=sha256(image).hexdigest(),
                byte_identity=True,image_provenance="not inferred",guest_execution=False)

def audit(media_root,loaded_image=None,loaded_cs=None):
    check((loaded_image is None)==(loaded_cs is None),"supply loaded image and CS together")
    from capstone import Cs,CS_ARCH_X86,CS_MODE_16
    from overlay_inventory import find_units,parse_mz
    e=(media_root/'START.EXE').read_bytes();o=(media_root/'GAME.OVR').read_bytes()
    check(sha256(e).hexdigest()==EXE_SHA and sha256(o).hexdigest()==OVR_SHA,"media changed")
    u=next(u for u in find_units(e[parse_mz(e)['header_bytes']:],o) if u['stub_para']==0x3C6)
    check(u['ovr_offset']==0x3073A and u['entries'][1]==0,"owner changed")
    c=o[u['ovr_offset']:u['ovr_offset']+u['code_size']];code=c[ENTRY:ENTRY+2]
    check(sha256(c[:0xE3]).hexdigest()=="8163e99c171558dbcfb4462b4f8c665325eb34d79f4a513a6ba77a46957a57e6","owner SHA changed")
    check(sha256(code).hexdigest()==SOURCE_SHA,"instruction SHA changed")
    x=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(code,ENTRY))
    check([(i.address,i.mnemonic,i.op_str) for i in x]==[(ENTRY,'mov','sp, bp')],"instruction shape changed")
    check(sha256(c[0x64:0x65]).hexdigest()=="cfae0d4248f7142f7b17f826cd7a519280e312577690e957830d23dcf35a3fff","next lead changed")
    loaded=None if loaded_image is None else verify_loaded_image(loaded_image.read_bytes(),loaded_cs,ENTRY,code)
    return dict(target="U3C6 MOVSPBP0062",source_sha256=SOURCE_SHA,stop_ip=0x64,
                loaded_image=loaded,original_frame_join=False,producer_consistency_inferred=False,
                native_credit=0,full_c_credit=0,retf_policy_selected=False,
                next_lead=dict(ip=0x64,stop=0x65,implemented=False))

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--media-root',type=Path,required=True)
    p.add_argument('--loaded-image',type=Path);p.add_argument('--loaded-cs',type=lambda x:int(x,0));p.add_argument('--output',type=Path)
    a=p.parse_args()
    if (a.loaded_image is None)!=(a.loaded_cs is None):p.error('supply --loaded-image and --loaded-cs together')
    try:r=audit(a.media_root,a.loaded_image,a.loaded_cs)
    except (OSError,ValueError,ImportError) as error:p.exit(1,f'static audit failed: {error}\n')
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps(r,sort_keys=True))
if __name__=='__main__':main()
