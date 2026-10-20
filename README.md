# NeuroCube

Автономная игровая консоль для Waveshare RP2350-Touch-LCD-1.54: три игры с наклонами, отдельный «Шар ответов», русский интерфейс и рекорды.

Текущий статус: прошивка собирается и запускается на плате; USB, IMU и идентичность touch проверены. Физические жесты, сенсорные нажатия, визуальное качество LCD и работа от аккумулятора требуют ручной приёмки. Подарочный MVP пока не считается полностью принятым.

## Документация

- [Установка и сборка Windows](docs/build.md)
- [Инструкция игрока](docs/player-guide.md)
- [Архитектура и добавление игры](docs/architecture.md)
- [Подтверждённые аппаратные факты](docs/hardware.md)
- [Hardware Bridge и протокол](docs/bridge.md)
- [Прогресс и фактические проверки](docs/progress.md)
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
