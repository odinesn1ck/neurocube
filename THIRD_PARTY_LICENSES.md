# Third-party components

[MIT](LICENSE) covers original NeuroCube code and documentation. It does not replace the licenses of third-party code, generated font data, SDK or linked runtimes. Original third-party copyright and permission notices remain in their files.

| Component | Origin / version | License / distribution material |
|---|---|---|
| Waveshare board reference and PIO | RP2350-Touch-LCD-1.54, `51a7fe28c1e5dd19a089a062980a275709e94df6` | Upstream Apache-2.0; [license](assets/Waveshare-LICENSE.txt), [additional DEV_Config notice](assets/NOTICE.txt) |
| ES8311 codec | Espressif esp-bsp `f259a6abd5808019b1903a422092a4234ca95a51`, portable adaptation in `third_party/espressif_es8311` | Apache-2.0, no processor-vendor restriction; [license](third_party/espressif_es8311/LICENSE), [provenance / changes](third_party/espressif_es8311/README.md), [audit](docs/evidence/licenses-1.0.0.md) |
| Noto Sans Mono bitmap subset | Checked-in `src/presentation/font_data.hpp`, generated with `tools/generate_font.py` | SIL OFL-1.1; [copyright and license](assets/OFL.txt). Font data remains OFL, not MIT |
| QR generator (development only) | python-qrcode 8.2; static matrix contains the project's URL | BSD-3-Clause and included Arase MIT provenance; [license](assets/licenses/qrcode-LICENSE.txt). Generator code is not embedded in firmware |
| Raspberry Pi Pico SDK | 2.3.1, `079c6f39023649b154152db30f1d781e884879bc` | BSD-3-Clause; [license](assets/licenses/Pico-SDK-LICENSE.txt) |
| TinyUSB | `86ad6e56c1700e85f1c5678607a762cfe3aa2f47` | MIT; [license](assets/licenses/TinyUSB-LICENSE.txt) |
| CMSIS headers | Pico SDK's CMSIS stub | Apache-2.0; [license](assets/licenses/CMSIS-LICENSE.txt) |
| Pico printf | SDK implementation based on Marco Paland | MIT; [license](assets/licenses/Pico-printf-LICENSE.txt) |
| Arm linked C/C++ runtimes | Arm GNU 14.2.Rel1, GCC 14.2.1, newlib 4.4.0 | [GCC GPLv3](assets/licenses/GCC-COPYING3.txt) with [Runtime Library Exception 3.1](assets/licenses/GCC-COPYING.RUNTIME.txt); [newlib aggregate notices](assets/licenses/Newlib-COPYING.txt). The compiler is not redistributed |
| Windows linked runtimes | LLVM-MinGW 20261006, msvcrt x86_64 | [Apache-2.0 with LLVM exceptions](assets/licenses/LLVM-LICENSE.txt); [MinGW notices](assets/licenses/MinGW-COPYING.txt) and companion COPYING files in `assets/licenses` |

[Notice provenance and SHA256](assets/licenses/sources.json). Release packaging includes these notices alongside MIT, OFL, Waveshare notices and the Apache ES8311 license, attribution, change notice and source provenance. Original images/music/factory firmware are not included. The graphics and IT stories are project-generated code/data; there are no licensed Star Wars logos, soundtrack or film assets.

Build-only Python/CMake/Ninja/picotool/Git tools are installed separately; their executables are not shipped with the firmware/simulator. Current ES8311 source permission is confirmed under Apache-2.0, including RP2350. Stable 1.0.0 remains **BLOCKED** pending replacement hardware acceptance and an owner decision on historical publications. The new license does not retroactively relicense old Git objects/assets; [history assessment](docs/evidence/es8311-license-replacement.md).
