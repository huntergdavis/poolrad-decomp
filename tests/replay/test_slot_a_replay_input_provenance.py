#!/usr/bin/env python3
"""A synthetic fixture must never acquire measured-native status by relabeling."""
import copy
import json
import unittest
from check_slot_a_replay_input_provenance import TABLE,validate
class ProvenanceConsistency(unittest.TestCase):
 def test_reject_synthetic_row_mislabelled_measured(self):
  data=json.loads(TABLE.read_text());validate(data)
  bad=copy.deepcopy(data)
  row=next(r for r in bad['rows'] if r['id']=='synthetic_ced_call_input')
  row['classification']='measured'
  with self.assertRaisesRegex(ValueError,'measured'):validate(bad)
if __name__=='__main__':unittest.main()
