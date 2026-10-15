import importlib.util
from hashlib import sha256
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("candidate",ROOT/"tools/overlay/u3c6_mov_sp_bp_0062_source_audit.py")
candidate=importlib.util.module_from_spec(spec);spec.loader.exec_module(candidate)

class LoadedBytes(unittest.TestCase):
    def image(self,cs=0x1404,ip=None,code=b"XY"):
        ip=candidate.ENTRY if ip is None else ip
        image=bytearray(1048576)
        for i,value in enumerate(code):image[(cs*16+ip+i)%len(image)]=value
        return image
    def test_exact(self):
        r=candidate.verify_loaded_image(self.image(),0x1404,candidate.ENTRY,b"XY")
        self.assertEqual(r["loaded_instruction_sha256"],sha256(b"XY").hexdigest())
    def test_a20_wrap(self):
        r=candidate.verify_loaded_image(self.image(0xFFFF,0x10),0xFFFF,0x10,b"XY")
        self.assertEqual(r["byte_addresses"],[0,1])
    def test_flip(self):
        image=self.image();image[(0x1404*16+candidate.ENTRY+1)%len(image)]^=1
        with self.assertRaises(ValueError):candidate.verify_loaded_image(image,0x1404,candidate.ENTRY,b"XY")
    def test_short(self):
        with self.assertRaises(ValueError):candidate.verify_loaded_image(bytes(1048575),0,0,b"XY")
    def test_long(self):
        with self.assertRaises(ValueError):candidate.verify_loaded_image(bytes(1048577),0,0,b"XY")
    def test_invalid_cs(self):
        for cs in (-1,65536,"0",True):
            with self.subTest(cs=cs),self.assertRaises(ValueError):candidate.verify_loaded_image(bytes(1048576),cs,0,b"XY")
    def test_invalid_ip(self):
        for ip in (-1,65536,"0",True):
            with self.subTest(ip=ip),self.assertRaises(ValueError):candidate.verify_loaded_image(bytes(1048576),0,ip,b"XY")
    def test_offset_cross(self):
        with self.assertRaises(ValueError):candidate.verify_loaded_image(bytes(1048576),0,65535,b"XY")
    def test_empty(self):
        with self.assertRaises(ValueError):candidate.verify_loaded_image(bytes(1048576),0,0,b"")
    def test_overlong_instruction(self):
        with self.assertRaises(ValueError):candidate.verify_loaded_image(bytes(1048576),0,0,bytes(16))
    def test_last_approved_word(self):
        image=self.image(0xFFFF,65534)
        self.assertTrue(candidate.verify_loaded_image(image,0xFFFF,65534,b"XY")["byte_identity"])
    def test_paired_arguments(self):
        for image,cs in ((None,0),(Path("absent"),None)):
            with self.subTest(image=image,cs=cs),self.assertRaises(ValueError):candidate.audit(Path("absent"),image,cs)
    def test_wrong_cs(self):
        with self.assertRaises(ValueError):candidate.verify_loaded_image(self.image(),0x1405,candidate.ENTRY,b"XY")
    def test_no_provenance_inference(self):
        image=self.image();before=bytes(image)
        r=candidate.verify_loaded_image(image,0x1404,candidate.ENTRY,b"XY")
        self.assertEqual(r["loaded_image_sha256"],sha256(before).hexdigest())
        self.assertEqual(r["image_provenance"],"not inferred");self.assertFalse(r["guest_execution"])
        self.assertEqual(bytes(image),before)

if __name__=="__main__":unittest.main()
