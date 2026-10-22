import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

spec=importlib.util.spec_from_file_location('bridge',Path(__file__).parents[1]/'tools/neurocube.py')
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)

def block(address=0x10000000,family=b.FAMILY_ARM_S,index=0,count=1):
    data=bytearray(512)
    struct.pack_into('<8I',data,0,0x0A324655,0x9E5D5157,0x2000,address,256,index,count,family)
    struct.pack_into('<I',data,508,0x0AB16F30)
    return data

class BridgeTests(unittest.TestCase):
    def validate(self,data):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'firmware.uf2';p.write_bytes(data);return b.validate_uf2(p)
    def test_valid_uf2(self):
        self.assertEqual(self.validate(block())['blocks'],1)
    def test_reject_wrong_family(self):
        with self.assertRaises(b.BridgeError):self.validate(block(family=0xE48BFF56))
    def test_reject_records_overlap(self):
        with self.assertRaises(b.BridgeError):self.validate(block(address=b.STORAGE_START))
    def test_reject_incomplete_and_duplicate(self):
        with self.assertRaises(b.BridgeError):self.validate(block(count=2))
        with self.assertRaises(b.BridgeError):self.validate(block(count=2)+block(count=2))
    def test_reject_bad_magic(self):
        data=block();data[0]=0
        with self.assertRaises(b.BridgeError):self.validate(data)
    def test_workaround(self):
        w=block(address=0x100FFF00,family=0xE48BFF57,index=0,count=2)
        struct.pack_into('<I',w,8,0xA000);w[32:288]=bytes([0xEF])*256;struct.pack_into('<I',w,288,0x9957E304)
        self.assertEqual(self.validate(w+block())['blocks'],2)
        w[288]=0
        with self.assertRaises(b.BridgeError):self.validate(w+block())
    def test_serial_selection(self):
        p=SimpleNamespace(serial_number='target',device='COM89')
        with patch.object(b,'ports',return_value=[p]):self.assertIs(b.select_port('target'),p)
        with patch.object(b,'ports',return_value=[p,p]):
            with self.assertRaises(b.BridgeError):b.select_port('target')
        with patch.object(b,'ports',return_value=[p]):
            with self.assertRaises(b.BridgeError):b.select_port('other')
    def test_flash_requires_authorization_before_usb(self):
        with patch.object(b,'pnp') as pnp:
            with self.assertRaises(b.BridgeError):b.flash('missing.uf2','target',False)
            pnp.assert_not_called()
    def test_lock_exclusion_and_recovery(self):
        with b.exclusive('unit-test'):
            with self.assertRaises(b.BridgeError):
                with b.exclusive('unit-test'):pass
        with b.exclusive('unit-test'):pass
    def test_protocol_parser(self):
        c=b.Connection('target',timeout=.1)
        stream=iter(b'noise\n{"v":2}\n{"v":1,"type":"status"}\n')
        c.port=SimpleNamespace(read=lambda n:bytes([next(stream)]))
        self.assertEqual(c.read()['type'],'status')
    def test_protocol_command_injection_rejected(self):
        c=b.Connection('target')
        with self.assertRaises(b.BridgeError):c.request('hello\nreboot')

if __name__=='__main__':unittest.main()
