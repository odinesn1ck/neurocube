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

Архивы инструментов проверяются bootstrap по SHA256, Pico SDK — по закреплённому commit. Инструменты устанавливаются отдельно и не входят в пользовательский релиз.

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
.\build-native\neurocube_sim.exe --gallery artifacts/gallery
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

[Workflow](../.github/workflows/build.yml) запускается при push в main/codex/neurocube-mvp и для pull request. На Windows runner он устанавливает закреплённые инструменты, проверяет документацию, запускает native/Python-тесты и headless-симулятор, собирает production UF2/ELF/BIN. Результаты доступны в артефакте `neurocube-candidate-<commit>` соответствующего run в GitHub Actions. Workflow не прошивает устройство и не создаёт релиз автоматически. Для [ручного workflow_dispatch](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow) GitHub требует наличие workflow в default branch; до согласованного merge используйте push/PR или повторный запуск существующего run. Двоичные сборки не коммитятся в git. Для локальной диагностики используйте tools/neurocube.py; CLI не поддерживает сборку на Linux, хотя переносимый core собирается обычным CMake и на Linux.

Bridge выполняет configure с --fresh: это исключает остаточные compiler flags от прерванного первоначального configure. Независимая проверка сравнивает UF2 из двух build-каталогов при одном коммите и дате сборки.

Матрица QR включена в исходники; обычная сборка не требует qrcode или ZXing. Для регенерации разработчик устанавливает `qrcode==8.2` и запускает `python tools/generate_qr.py`. При проверке кадра использован независимый ZXing-C++ 2.3.0. ARM-сборка создаёт `.su` для контроля стека; core0 резервирует 4096 байт, крупные рабочие буферы размещены в .bss.

Артефакт push относится к commit ветки, PR-артефакт — к временному merge commit, который GitHub создаёт только для проверки. Это не merge PR в main. Версия Git в firmware поэтому отличается; CI-сборка не заменяет опубликованный и аппаратно проверенный релиз автоматически.

В оконном симуляторе клавиша P моделирует PWR: удержание → «Выключаюсь», отпускание → имитация оставшегося внешнего питания. GPIO не вызываются; это не аппаратная проверка выключения.

## Воспроизводимая production-поставка

Версия задаётся только `project(... VERSION ...)` в CMake; USB hello получает тот же NEUROCUBE_VERSION. `tool_environment` устанавливает SOURCE_DATE_EPOCH из даты исходного коммита; при ручном configure/build задавайте одну и ту же переменную для обеих стадий. Префиксы checkout/build/SDK удаляются из компилируемых путей, у Windows PE отключён меняющийся timestamp.

```powershell
& $python tools/prepare_release.py --verify-reproducible
```

Нужно чистое рабочее дерево. Команда сравнивает замороженные игровые/header/board/display файлы с `v0.3.5-rc1`, собирает production UF2/ELF/BIN и Release Windows simulator дважды в независимых каталогах, сравнивает SHA256, проверяет debug OFF и адреса UF2. Создаёт ZIP с исходниками, общий ZIP с лицензиями/инструкцией/manifest и отдельный Windows ZIP; внешние SHA256SUMS создаются после архивов. Codec driver — закреплённая Apache-2.0 адаптация из esp-bsp; её transport glue закреплён SHA256 в provenance. Старые codec sources и громкий RAM probe запрещены в HEAD и упаковке. Никакого flash, tag, GitHub release или публикации нет. Рабочие сборочные каталоги остаются в игнорируемом artifacts.

Для стабильного 1.0 остаются критерии [готовности](evidence/release-1.0.0-readiness.md), включая лицензионный блокер ES8311. Пакет с blocked manifest не означает разрешение публикации.

## Резервная копия перед разрешённой прошивкой

Сначала `detect`/`info`, свежие `records`/`settings` вне Git, затем точная сверка serial и USB topology. После разрешённого перехода в BOOTSEL сохранить диапазон `0x10FFE000..0x11000000` через `picotool save -r ... -v` в файл **вне репозитория**, проверить 8192 байта и SHA256. Bridge `flash --allow-flash` принимает уже идентифицированный BOOTSEL и выполняет readback verify. После запуска сравнить все scores, sequence, calibration, sensitivity и sound_enabled с исходным состоянием. Сам `flash` не заменяет предварительный backup. Никогда не применять erase-all или полный factory image для обычного обновления.

Для1.0.1 builder сравнивает замороженные файлы с тегомv1.0.0; только render-копирайт и один глиф© разрешены точными SHA256 в tools/release-freeze-1.0.1.json. Новая UI-правка не ослабляет проверки board/power/audio. По умолчанию пакет создаётся в artifacts/release-candidate/dist.
