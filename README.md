# NeuroCube

NeuroCube — автономная карманная консоль с короткими играми на внимание, память, реакцию и координацию. Управление наклонами, поворотами, сенсором и кнопкой связывает игровые задачи с физическим движением. Проект задуман как персональный подарок Андрею с приветствием «Привет, Андрей».

Платформа: **Waveshare RP2350-Touch-LCD-1.54**, сенсорный экран 240×240, IMU, аккумулятор и USB-C. Игры работают без сети и аккаунта. Три сета — «Фокус», «Координация», «Синхронизация» — рассчитаны на 12 игр. Реализованы Simon/«Повтори ритм», лабиринт, «Лови команду» и отдельный «Шар ответов» с парными классическими и IT-фразами. Полный замысел и отличие планов от реализации описаны в документах ниже.

## Порядок знакомства

1. [Продуктовый замысел и каталог](docs/product-vision.md) — зачем существует проект, механики, персонализация и утверждённые тексты.
2. [Правила разработки для агентов](AGENTS.md) и [политика документации](docs/documentation-policy.md).
3. [Архитектура](docs/architecture.md), [аппаратная конфигурация](docs/hardware.md) и [USB Hardware Bridge](docs/bridge.md).
4. [Текущее состояние и проверки](docs/progress.md), [задачи и дефекты](docs/backlog.md), [история изменений](CHANGELOG.md).

[Участие в разработке](CONTRIBUTING.md) · [MIT собственного кода](LICENSE) · [Сторонние компоненты](THIRD_PARTY_LICENSES.md)

[Инструкция игрока](docs/player-guide.md) · [Сборка и установка инструментов](docs/build.md) · [Ручная приёмка](docs/morning-checklist.md) · [Карта документации](docs/README.md) · [Сборки](https://github.com/odinesn1ck/neurocube/releases)

## Быстрый запуск разработчика

В Windows с Python 3.12 и Git:

```powershell
git clone https://github.com/odinesn1ck/neurocube.git
cd neurocube
.\tools\bootstrap.ps1
$python = "$env:USERPROFILE\neurocube-tools\venv\Scripts\python.exe"
& $python tools/neurocube.py build --native
& $python -m unittest discover -s tests -p 'test_*.py'
.\build-native\neurocube_sim.exe
```

Симулятор использует то же ядро C++17, что и RP2350: стрелки — наклон, Q — поворот, пробел — встряхивание, мышь — сенсор, Tab — следующая игра, Esc — меню. [Инструкция сборки UF2 и обновления платы](docs/build.md#прошивка) описывает отдельный аппаратный этап.

Оригинальный код NeuroCube распространяется по MIT. Сторонние компоненты сохраняют собственные лицензии; их происхождение и уведомления перечислены отдельно. [Подготовка и критерии выпуска](docs/evidence/release-1.0.0-readiness.md).
