"""Synthetic fail-closed controls; no original media or original bytes."""
import copy
from pathlib import Path
import tempfile
import unittest

from verify_scrubbed_spans import sha, verify


class ScrubbedSpanTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / 'docs/re').mkdir(parents=True)
        self.data = b'synthetic evidence fixture, not game content'
        self.media = {'START.EXE': self.data}
        self.pins = {'START.EXE': sha(self.data)}
        self.row = dict(doc='docs/re/example.md', span='example-1',
                        sha256=sha(self.data[3:12]), length='9',
                        media='START.EXE', file_offset='0x3', status='resolved',
                        attribution='synthetic fixture')
        self.write_marker()

    def write_marker(self, row=None, suffix=''):
        row = row or self.row
        offset = row['media'] + ':' + row['file_offset'] if row['status'] == 'resolved' else 'UNRESOLVED'
        text = f"[scrubbed: span={row['span']} sha256={row['sha256']} bytes={row['length']} offset={offset}]"
        (self.root / row['doc']).write_text(text + suffix)

    def check(self, rows=None, media=None):
        return verify(self.root, [self.row] if rows is None else rows,
                      self.media if media is None else media, self.pins)

    def test_match(self):
        self.assertEqual(self.check()['status'], 'PASS')

    def test_unresolved_is_suspended(self):
        row = dict(self.row, status='unresolved', media='', file_offset='')
        self.write_marker(row)
        result = self.check([row])
        self.assertEqual(result['status'], 'SUSPENDED')
        self.assertEqual(result['verified'], [])

    def test_wrong_offset_even_with_consistent_marker_fails(self):
        row = dict(self.row, file_offset='0x4')
        self.write_marker(row)
        self.assertEqual(self.check([row])['status'], 'ERROR')

    def test_wrong_media_fails(self):
        self.assertEqual(self.check(media={'START.EXE': self.data + b'!'} )['status'], 'ERROR')

    def test_out_of_bounds_fails(self):
        row = dict(self.row, file_offset='-1')
        self.write_marker(row)
        self.assertEqual(self.check([row])['status'], 'ERROR')

    def test_stale_marker_fails(self):
        self.assertEqual(self.check([dict(self.row, length='8')])['status'], 'ERROR')

    def test_missing_row_cannot_skip_marker(self):
        self.assertEqual(self.check([])['status'], 'ERROR')

    def test_duplicate_manifest_fails(self):
        self.assertEqual(self.check([self.row, copy.copy(self.row)])['status'], 'ERROR')

    def test_duplicate_marker_fails(self):
        path = self.root / self.row['doc']
        path.write_text(path.read_text() * 2)
        self.assertEqual(self.check()['status'], 'ERROR')

    def test_malformed_marker_fails(self):
        self.write_marker(suffix=' [scrubbed: invalid]')
        self.assertEqual(self.check()['status'], 'ERROR')


if __name__ == '__main__':
    unittest.main()
