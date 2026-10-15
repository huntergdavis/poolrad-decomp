#!/usr/bin/env python3
"""Lightweight schema/lineage and optional read-only private-hash verification."""
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[2]
TABLE=ROOT/'docs/re/SLOT_A_REPLAY_INPUT_PROVENANCE.json'
HEX=re.compile(r'[0-9a-f]{64}')
PC=re.compile(r'[0-9A-F]{4}:[0-9A-F]{4}')
def require(ok,message):
 if not ok:raise ValueError(message)
def keys(value,expected,label):
 require(isinstance(value,dict) and set(value)==set(expected),label+' fields')
def validate(data):
 keys(data,('schema_version','media_hashes','rows'),'table')
 require(data['schema_version']==1,'schema version')
 keys(data['media_hashes'],('start_exe','game_overlay','saved_a'),'media hashes')
 require(all(isinstance(v,str) and HEX.fullmatch(v) for v in data['media_hashes'].values()),'media digest')
 require(isinstance(data['rows'],list) and bool(data['rows']),'empty table')
 by_id={}
 for row in data['rows']:
  keys(row,('id','classification','pc','condition','derived_from','state_snapshot','evidence','hashes'),'row')
  ident=row['id'];require(isinstance(ident,str) and re.fullmatch(r'[a-z0-9_]+',ident),'row id')
  require(ident not in by_id,'duplicate id');by_id[ident]=row
  require(isinstance(row['pc'],str) and PC.fullmatch(row['pc']),'loaded PC')
  require(isinstance(row['derived_from'],list) and all(isinstance(p,str) for p in row['derived_from']),'parents')
  e=row['evidence'];h=row['hashes'];kind=row['classification']
  if kind=='measured':
   keys(e,('kind','capture','cpu_row_index'),'measured witness')
   require(e['kind']=='native_capture' and not row['derived_from'],'measured row requires a direct native_capture witness, never a synthetic derivative')
   require(row['condition'] in ('baseline_file_set','controlled_file_absence') and row['state_snapshot']=='retained','measured condition/snapshot')
   require(isinstance(e['capture'],str) and re.fullmatch(r'[a-z0-9-]+',e['capture']),'capture id')
   require(type(e['cpu_row_index']) is int and e['cpu_row_index']>=0,'CPU row index')
   keys(h,('cpu_json','ram','raw_transcript'),'measured hashes')
  elif kind=='original_produced_static':
   keys(e,('kind','source_commit','source_path','source_pc'),'static witness')
   require(e['kind']=='original_execution_report' and row['condition']=='static_original_execution','static evidence')
   require(row['state_snapshot']=='not_retained' and len(row['derived_from'])==1,'static output retention/seed')
   require(isinstance(e['source_pc'],str) and PC.fullmatch(e['source_pc']),'static source PC')
   keys(h,('source_code','execution_log','seed_cpu_json','seed_ram'),'static hashes')
  elif kind=='synthetic':
   keys(e,('kind','source_commit','source_path'),'synthetic witness')
   require(e['kind'] in ('c_fixture','static_control_fixture') and row['condition']=='synthetic_fixture','synthetic origin')
   require(row['state_snapshot']=='not_retained','synthetic snapshot retention')
   keys(h,('fixture_source','test_log'),'synthetic hashes')
  else:raise ValueError('unknown classification')
  require(all(isinstance(v,str) and HEX.fullmatch(v) for v in h.values()),'artifact digest')
  if kind!='measured':
   require(isinstance(e['source_commit'],str) and re.fullmatch(r'[0-9a-f]{40}',e['source_commit']),'source commit')
   require(isinstance(e['source_path'],str) and e['source_path'].startswith('tests/replay/') and '..' not in Path(e['source_path']).parts,'public source path')
 for row in data['rows']:
  for parent in row['derived_from']:require(parent in by_id,'missing parent')
  if row['classification']=='original_produced_static':
   parent=by_id[row['derived_from'][0]];h=row['hashes']
   require(parent['classification']=='measured' and parent['pc']==row['evidence']['source_pc'],'static measured seed')
   require(h['seed_cpu_json']==parent['hashes']['cpu_json'] and h['seed_ram']==parent['hashes']['ram'],'static seed hash mismatch')
 visiting=set();done=set()
 def visit(ident):
  require(ident not in visiting,'cyclic provenance')
  if ident in done:return
  visiting.add(ident)
  for parent in by_id[ident]['derived_from']:visit(parent)
  visiting.remove(ident);done.add(ident)
 for ident in by_id:visit(ident)

def verify_private(data,manifest):
 private=(ROOT/'resources').resolve();manifest=Path(manifest).resolve(strict=True)
 require(manifest.is_relative_to(private),'private manifest must stay in resources/')
 records=json.loads(manifest.read_text());lookup={}
 for item in records:
  keys(item,('path','sha256'),'private artifact')
  p=Path(item['path']).resolve(strict=True);require(p.is_relative_to(private),'private artifact path')
  require(sha256(p.read_bytes()).hexdigest()==item['sha256'],'private hash mismatch')
  lookup[item['sha256']]=p
 needed=set(data['media_hashes'].values())
 for row in data['rows']:needed.update(row['hashes'].values())
 require(needed<=set(lookup),'private manifest missing pinned artifacts')
 names='cs ip ds es fs gs ss sp bp ax bx cx dx si di flags'.split()
 for row in data['rows']:
  if row['classification']!='measured':continue
  h=row['hashes'];cpu=json.loads(lookup[h['cpu_json']].read_text())
  raw=lookup[h['raw_transcript']].read_bytes()
  clean=re.sub(re.escape(chr(27)+'[')+r'[0-?]*[ -/]*[@-~]','',raw.decode(errors='replace'))
  rows=re.findall("LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\\s*LOG: ([0-9a-f ]+)",clean)
  index=row['evidence']['cpu_row_index'];require(index<len(rows),'raw CPU row missing')
  require([cpu[k] for k in names]==[int(x,16) for x in rows[index].split()],'CPU/raw mismatch')
  require(row['pc']==f"{cpu['cs']:04X}:{cpu['ip']:04X}",'measured PC mismatch')
 return len(records)
def main():
 if len(sys.argv)>2:raise SystemExit('usage: checker [IGNORED_PRIVATE_MANIFEST]')
 data=json.loads(TABLE.read_text());validate(data)
 count=verify_private(data,sys.argv[1]) if len(sys.argv)==2 else 0
 print(f'PASS {len(data["rows"])} provenance rows;{count} read-only private artifact checks;no execution or parity credit')
if __name__=='__main__':main()
