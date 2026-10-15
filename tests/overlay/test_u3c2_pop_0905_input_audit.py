"""Synthetic image metadata only; no original media or guest execution."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest

TOOL = Path(__file__).resolve().parents[2] / "tools/overlay/u3c2_pop_0905_input_audit.py"
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

class StackInputTests(unittest.TestCase):
    def test_boundary_address_requirements(self):
        cases=[(0,0,[0,1]),(0x2000,0xFFFF,[0x2FFFF,0x30000]),
               (0xFFFF,15,[0xFFFFF,0]),(0xFFFF,0xFFFF,[0xFFEF,0xFFF0])]
        for ss,sp,want in cases:
            with self.subTest(ss=ss,sp=sp):
                result=AUDIT.required_stack_addresses(ss,sp)
                for candidate in result["candidates"].values():
                    self.assertEqual(candidate["input_word_byte_addresses"],want)
                self.assertFalse(result["candidate_addresses_differ"])
                self.assertIsNone(result["selected_policy"])
                self.assertFalse(result["native_wrap_policy_resolved"])
    def test_offset_boundary_marker(self):
        self.assertFalse(AUDIT.required_stack_addresses(0x1234,0xFFFE)["input_word_straddles_offset_boundary"])
        self.assertTrue(AUDIT.required_stack_addresses(0x1234,0xFFFF)["input_word_straddles_offset_boundary"])
    def test_strict_ss_sp_domain(self):
        for bad in [True,False,-1,65536,1.0,"0",None]:
            for ss,sp in [(bad,0),(0,bad)]:
                with self.subTest(ss=ss,sp=sp),self.assertRaises(ValueError):
                    AUDIT.required_stack_addresses(ss,sp)
    def test_no_stack_reads_pop_values_or_cpu_outputs(self):
        result=AUDIT.required_stack_addresses(0xFFFF,0xFFFF)
        for key in ["stack_contents_read","pop_value_read","cpu_transition_evaluated"]:
            self.assertFalse(result[key])
        for key in ["BP","post_pop_SP","next_IP","cpu","word_value"]:
            self.assertNotIn(key,result)
    def test_api_pairing_before_media_access(self):
        for ss,sp in [(0,None),(None,0)]:
            with self.assertRaisesRegex(ValueError,"supply SS and SP together"):
                AUDIT.audit(Path("/nonexistent-no-game-media"),ss=ss,sp=sp)
    def test_cli_ss_sp_pairing(self):
        for arg in ["--ss","--sp"]:
            result=subprocess.run([sys.executable,str(TOOL),arg,"0"],capture_output=True,text=True)
            self.assertEqual(result.returncode,2)
            self.assertIn("supply --ss and --sp together",result.stderr)

if __name__ == "__main__":
    unittest.main()
