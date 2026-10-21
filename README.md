# NeuroCube

## Что это за проект

**NeuroCube** — автономная карманная консоль с играми на внимание, память, реакцию и координацию. Игрок управляет наклонами, поворотами, сенсором и физической кнопкой. Проект задуман как персональный подарок Андрею; при включении предусмотрено приветствие «Привет, Андрей». Три тематических сета («Фокус», «Координация», «Синхронизация») рассчитаны на расширение до 12 игр; первый этап предусматривает три основные игры и отдельный режим «Шар ответов» с классическими и IT-ироничными ответами. Игры вдохновлены известными механиками, но не копируют чужое оформление.

**Продуктовые требования:** [product-vision](docs/product-vision.md). **Архитектура:** [architecture](docs/architecture.md). **Оперативные задачи и дефекты:** [backlog](docs/backlog.md). **История изменений:** [CHANGELOG](CHANGELOG.md).

### Порядок чтения для нового разработчика или агента

1. [Продуктовый замысел и каталог игр](docs/product-vision.md) — что и зачем строим, UX, правила и словарь «Шара».
2. [AGENTS.md](AGENTS.md) — обязательные инженерные правила.
3. [Архитектура](docs/architecture.md) и [аппаратная конфигурация](docs/hardware.md) — как реализовано.
4. [Бэклог](docs/backlog.md), [прогресс](docs/progress.md) и [история изменений](CHANGELOG.md) — что предстоит, что проверено и что менялось.

## Документация

- [Установка и сборка Windows](docs/build.md)
- [Инструкция игрока](docs/player-guide.md)
- [Архитектура и добавление игры](docs/architecture.md)
- [Подтверждённые аппаратные факты](docs/hardware.md)
- [Hardware Bridge и протокол](docs/bridge.md)
- [Прогресс и фактические проверки](docs/progress.md)
- [Бэклог](docs/backlog.md)
- [История изменений](CHANGELOG.md)
- [Утренний чек-лист](docs/morning-checklist.md)

## Быстрый запуск разработчика

```powershell
.\tools\bootstrap.ps1
$python = "$env:USERPROFILE\neurocube-tools\venv\Scripts\python.exe"
& $python tools/neurocube.py build --native
& $python -m unittest discover -s tests -p 'test_*.py' -v
& $python tools/neurocube.py build
& $python tools/neurocube.py detect
& $python tools/neurocube.py info
```

Прошивка является отдельной операцией и заменяет существующую программу:

```powershell
& $python tools/neurocube.py flash build-pico/neurocube.uf2 --allow-flash
```

Симулятор: `build-native/neurocube_sim.exe`. Стрелки — наклон, Q — поворот, пробел — встряхивание, мышь — сенсор, Tab — следующая игра, Esc — меню. Нативное приложение использует то же игровое ядро, что и прошивка.
