"""Synthetic image metadata only; no original media or guest execution."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest

TOOL = Path(__file__).resolve().parents[2] / "tools/overlay/u3c2_cmp_al_stack_060b_static_audit.py"
SPEC = importlib.util.spec_from_file_location("loaded_audit", TOOL)
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)

class LoadedImageTests(unittest.TestCase):
    def image(self, cs=0x1404, entry=0x1234, code=b"\x12\x34\x56\x78"):
        ram = bytearray(1048576)
        for i, value in enumerate(code):
            ram[(16 * cs + entry + i) % 1048576] = value
        return ram
    def test_exact_bytes_and_no_mutation(self):
        ram=self.image();before=bytes(ram)
        result=AUDIT.verify_loaded_image(ram,0x1404,0x1234,b"\x12\x34\x56\x78")
        self.assertTrue(result["loaded_instruction_bytes_match_source"])
        self.assertFalse(result["guest_execution_or_frame_join"])
        self.assertEqual(bytes(ram),before)
    def test_a20_crossing(self):
        result=AUDIT.verify_loaded_image(self.image(0xFFFF,14),0xFFFF,14,b"\x12\x34\x56\x78")
        self.assertEqual(result["byte_addresses"],[1048574,1048575,0,1])
    def test_each_wrong_byte_rejected(self):
        for i in range(4):
            ram=self.image();ram[0x14040+0x1234+i]^=1
            with self.subTest(i=i),self.assertRaises(ValueError):
                AUDIT.verify_loaded_image(ram,0x1404,0x1234,b"\x12\x34\x56\x78")
    def test_wrong_load_position(self):
        with self.assertRaises(ValueError):
            AUDIT.verify_loaded_image(self.image(),0x1404,0x1235,b"\x12\x34\x56\x78")
    def test_image_lengths_rejected(self):
        for size in (0,1048575,1048577):
            with self.subTest(size=size),self.assertRaises(ValueError):
                AUDIT.verify_loaded_image(bytes(size),0,0,b"a")
    def test_invalid_cs(self):
        for value in (-1,65536,True,None):
            with self.subTest(value=value),self.assertRaises(ValueError):
                AUDIT.verify_loaded_image(self.image(),value,0x1234,b"ab")
    def test_invalid_entry(self):
        for value in (-1,65536,True,None):
            with self.subTest(value=value),self.assertRaises(ValueError):
                AUDIT.verify_loaded_image(self.image(),0x1404,value,b"ab")
    def test_width_or_offset_crossing_rejected(self):
        for entry,code in ((0,b""),(0,bytes(16)),(65535,b"ab")):
            with self.subTest(entry=entry,code=code),self.assertRaises(ValueError):
                AUDIT.verify_loaded_image(self.image(),0,entry,code)
    def test_unpaired_audit_inputs_reject_before_media(self):
        for image,cs in ((Path("missing"),None),(None,0x1404)):
            with self.subTest(image=image,cs=cs),self.assertRaisesRegex(ValueError,"together"):
                AUDIT.audit(Path("missing-root"),image,cs)
    def test_cli_unpaired_rejects_before_media(self):
        for args in (("--loaded-image","missing"),("--loaded-cs","0x1404")):
            result=subprocess.run([sys.executable,str(TOOL),"--root","missing-root",*args],capture_output=True,text=True)
            self.assertEqual(result.returncode,2)
            self.assertIn("together",result.stderr)
            self.assertEqual(result.stdout,"")
    def test_other_image_bytes_do_not_imply_frame_equality(self):
        ram=self.image();ram[0]=99
        result=AUDIT.verify_loaded_image(ram,0x1404,0x1234,b"\x12\x34\x56\x78")
        self.assertTrue(result["loaded_instruction_bytes_match_source"])
        self.assertIn("not inferred",result["image_origin"])
        self.assertFalse(result["guest_execution_or_frame_join"])

if __name__ == "__main__":
    unittest.main()
