from pathlib import Path
import re
import unittest
ROOT=Path(__file__).parents[1]
class ArchitectureTests(unittest.TestCase):
    def test_portable_layers_have_no_hardware_dependencies(self):
        for layer in ('domain','application','games','presentation'):
            for path in (ROOT/'src'/layer).glob('*.cpp'):
                text=path.read_text(encoding='utf-8')
                self.assertNotRegex(text,r'#\s*include\s*[<"](?:pico/|hardware/|windows\.h|neurocube/board\.hpp)',str(path))
    def test_games_do_not_import_application(self):
        for path in (ROOT/'src/games').glob('*.cpp'):
            self.assertNotIn('neurocube/application.hpp', path.read_text(encoding='utf-8'))
    def test_inner_headers_have_no_hardware_dependencies(self):
        for name in ('domain.hpp','application.hpp','games.hpp','framebuffer.hpp','ui.hpp','config.hpp','idle.hpp','crawl.hpp','audio.hpp'):
            text=(ROOT/'include/neurocube'/name).read_text(encoding='utf-8')
            self.assertNotRegex(text,r'#\s*include\s*[<"](?:pico/|hardware/|windows\.h)',name)
if __name__=='__main__':unittest.main()
