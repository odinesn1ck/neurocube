# Сборка NeuroCube на Windows

Инструменты устанавливаются в `%USERPROFILE%\neurocube-tools`, системный PATH и драйверы не меняются. Проверенная ОС — Windows Server 2019 x64, Python 3.12.8. Для другой папки установите NEUROCUBE_TOOLS и передайте тот же ToolRoot в bootstrap.

## Закреплённые версии

| Компонент | Версия |
|---|---|
| Pico SDK | 2.3.1, commit `079c6f39023649b154152db30f1d781e884879bc` |
| TinyUSB | submodule `86ad6e56c1700e85f1c5678607a762cfe3aa2f47` |
| ARM GNU | 14.2.Rel1, GCC 14.2.1 |
| picotool | 2.3.1, официальный Windows archive Raspberry Pi |
| LLVM-MinGW | 20261006, Clang 23.1.3, msvcrt x86_64 |
| CMake / Ninja | 3.31.6 / 1.11.1.3 |
| pyserial / Pillow | 3.5 / 11.1.0 |

ARM загружается с developer.arm.com; picotool, SDK и LLVM-MinGW — из release/source репозиториев разработчиков; Python-пакеты — из PyPI. В ночном прогоне ZIP Arm был скачан диапазонами после зависания одиночной передачи, проверен по официальному Content-MD5, затем распакован. Zig 0.14.1 также установлен как запасной нативный компилятор, но итоговая сборка его не использует.

## Установка

```powershell
.\tools\bootstrap.ps1
$python = "$env:USERPROFILE\neurocube-tools\venv\Scripts\python.exe"
```

Если политика PowerShell запрещает запуск скрипта, не отключайте защиту глобально: используйте существующий разрешённый способ запуска в своей среде. При прерывании загрузки проверьте архив и повторите установку; не используйте неполную распаковку toolchain.

## Тесты и симулятор

```powershell
& $python tools/neurocube.py build --native
& $python -m unittest discover -s tests -p 'test_*.py' -v
.\build-native\core_tests.exe
.\build-native\neurocube_sim.exe --headless build-native/menu.ppm
.\build-native\neurocube_sim.exe
```

C++ core и игры одинаковы для native и Pico. Windows executables статически линкуются, runtime DLL компилятора не нужны. Симулятор сохраняет рекорды в sim-records.bin в текущем каталоге; для изолированного прогона запускайте его из build-native.

## Прошивка

```powershell
& $python tools/neurocube.py build
& $python tools/neurocube.py flash build-pico/neurocube.uf2 --allow-flash
& $python tools/neurocube.py info
& $python tools/neurocube.py selftest
```

Выходы: build-pico/neurocube.uf2, .elf, .bin, .elf.map. UF2 перед записью проверяется по magic, family, адресам и полноте блоков; picotool выполняет readback verify. Отдельная debug-сборка: `build --debug`, каталог build-pico-debug. Она разрешает синтетический ввод и не предназначена для подарочного использования. Обычная сборка этот ввод не принимает.

Workflow подготовлен в docs/ci/build.yml, но не включён: GitHub отклонил push в .github/workflows из-за отсутствующего scope workflow у PAT. После расширения прав перенесите YAML в .github/workflows/build.yml. Готовые UF2, ELF, BIN и Windows-симулятор будут опубликованы файлами предварительного релиза. Двоичные сборки не коммитятся в git. Для локальной диагностики используйте tools/neurocube.py; CLI не поддерживает сборку на Linux, хотя переносимый core собирается обычным CMake и на Linux.
