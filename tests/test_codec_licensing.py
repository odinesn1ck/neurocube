from pathlib import Path
import hashlib
import json
import unittest

ROOT = Path(__file__).resolve().parents[1]

class CodecDistributionTests(unittest.TestCase):
    def test_restricted_driver_and_loud_probe_are_absent(self):
        for name in ('third_party/waveshare_audio/es8311.c', 'third_party/waveshare_audio/es8311.h',
                     'third_party/waveshare_audio/DEV_Config.h', 'third_party/waveshare_audio/audio_pio.h',
                     'tools/audio_probe/main.c', 'tools/audio_probe/CMakeLists.txt'):
            self.assertFalse((ROOT / name).exists(), name)
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        self.assertIn('third_party/espressif_es8311/es8311.c', cmake)
        self.assertNotIn('waveshare_audio/es8311', cmake)

    def test_current_codec_has_unrestricted_apache_provenance(self):
        directory = ROOT / 'third_party/espressif_es8311'
        p = json.loads((directory / 'provenance.json').read_text())
        self.assertEqual(p['commit'], 'f259a6abd5808019b1903a422092a4234ca95a51')
        self.assertEqual(p['license'], 'Apache-2.0')
        source_hashes = {s['path']: s['sha256'] for s in p['sources']}
        self.assertEqual(hashlib.sha256((directory / 'es8311_reg.h').read_bytes().replace(b'\r\n', b'\n')).hexdigest(), source_hashes['priv_include/es8311_reg.h'])
        for name in ('es8311.c', 'es8311_reg.h', 'es8311.h'):
            text = (directory / name).read_text()
            self.assertIn('SPDX-License-Identifier: Apache-2.0', text)
            self.assertNotIn('for use on all ESPRESSIF SYSTEMS products', text)
        self.assertIn('Modified by NeuroCube', (directory / 'es8311.c').read_text())

    def test_release_adapter_matches_reviewed_port(self):
        provenance = json.loads((ROOT / 'third_party/espressif_es8311/provenance.json').read_text())
        data = (ROOT / 'src/infrastructure/audio.cpp').read_bytes().replace(b'\r\n', b'\n')
        self.assertEqual(hashlib.sha256(data).hexdigest(), provenance['neurocube_audio_adapter_sha256'])

    def test_apache_notice_copy_and_inventory(self):
        notice = (ROOT / 'assets/licenses/ES8311-Apache-2.0.txt').read_bytes()
        original = (ROOT / 'third_party/espressif_es8311/LICENSE').read_bytes()
        self.assertEqual(notice, original)
        sources = json.loads((ROOT / 'assets/licenses/sources.json').read_text())
        entry = next(s for s in sources if s['file'] == 'ES8311-Apache-2.0.txt')
        self.assertEqual(hashlib.sha256(notice).hexdigest(), entry['sha256'])
        self.assertIn(b'Grant of Copyright License', notice)

if __name__ == '__main__':
    unittest.main()
