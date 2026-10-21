# Исправления ревью и генератор — 21.10.2026

Исходный [статический отчёт](code-review-2026-10-21.md) сохранён без изменения выводов о прежнем коде. Локальные незакоммиченные документы пользователя сохранены при fetch/stash/fast-forward/pop, резервная копия находится вне git.

## Реализовано

32-битный seed и случайный корень DFS, отдельные retry/new/next, явная кнопка «Новый лабиринт» в setup/pause/result, подпись seed. Процедурные карты показывают счёт попытки без записи глобального рекорда; v3, оба сектора и реальные результаты не мигрируют. Commands реагирует до завершения команды; настройка чувствительности циклична; штраф только за столкновение; меню использует нарисованные hitbox. Ранний BAT_EN HIGH сохранён, повторный gpio_init исключён по проверенному исходнику Pico SDK 2.3.1. Это не доказательство прежнего физического провала питания.

## Программные проверки

- 6000 seed, по 2000 на сложность: 1932/2000/2000 разных геометрий. Повторяемость, связность всех клеток, симметрия проходов, N−1 рёбер, границы массивов/стен и свободный проход шара радиуса 4.
- 300 маршрутов пройдены настоящей физикой MazeGame. Полный native suite: 87517642 assertions (большинство — сеточные проверки свободного коридора).
- Retry сохраняет seed/карту/настройки; new меняет карту; результат не меняет legacy best или maze_best. Полный цикл чувствительности переживает encode/decode, калибровка прежняя.
- 5+ игр: все вертикальные координаты и края hitbox на двух страницах, зазоры/невидимые кнопки. Приближение без удара, прижатие, rearm. Круг движется до ответа, угол стоит без поворота; нижняя граница не заходит под текст.
- 14 Python tests пройдены. [Галерея](../images/review-fixes-gallery-2026-10-21.png) из симулятора: retry побайтно равен первому кадру, new отличается; все шесть команд просмотрены.

## Аппаратная проверка и доставка

Production 0.3.1 из [ab338f5](https://github.com/odinesn1ck/neurocube/commit/ab338f55400488d31bc811ec1a8eefe97f5e114d) собрана и установлена на serial 419CB819454EA93D с readback verify. [USB smoke](review-usb-smoke-2026-10-21.json) пройден; рекорды, sequence 15 и настройки до/после совпали. Резервная копия 8192 байт сохранена вне git, SHA256 `55b044b430f28687b506c71112f0f0e0add0a63e505a82e2a989096efbc9e030`. Физический холодный старт/автономность, удобство новых экранов и жестов остаются NOT_TESTED до подтверждения пользователя. Актуальный установленный коммит и USB evidence — в [progress](../progress.md). [Короткий чек-лист](../morning-checklist.md#приёмка-исправлений-031).


[Релиз и UF2](https://github.com/odinesn1ck/neurocube/releases/tag/v0.3.1-rc1), [CI и проверка доставки](review-verification-2026-10-21.json). Восемь файлов релиза проверены по серверному SHA256; UF2 дополнительно скачан и побайтно сравнен с установленным образом. Native/Python/документация/ARM прошли также на GitHub Windows runner (push и PR).

ARM `.su`: main 424, App::update 72, App::launch 32, MazeGame::start 1592, make_maze 288 байт; известная цепочка генерации около 2408 байт против резерва 4096. Это статические кадры, не измеренный IRQ/high-water. Формат журнала, 24 пары Magic Ball, приветствие, QR и скринсейвер не изменены.

## Изменённые файлы

- `CHANGELOG.md`
- `CMakeLists.txt`
- `docs/architecture.md`
- `docs/backlog.md`
- `docs/evidence/README.md`
- `docs/evidence/code-review-2026-10-21.md`
- `docs/evidence/review-fixes-2026-10-21.md`
- `docs/hardware.md`
- `docs/images/review-fixes-gallery-2026-10-21.png`
- `docs/morning-checklist.md`
- `docs/player-guide.md`
- `docs/product-vision.md`
- `docs/progress.md`
- `docs/tasks/README.md`
- `include/neurocube/application.hpp`
- `include/neurocube/domain.hpp`
- `include/neurocube/games.hpp`
- `include/neurocube/ui.hpp`
- `src/application/app.cpp`
- `src/application/gestures.cpp`
- `src/application/registry.cpp`
- `src/games/games.cpp`
- `src/infrastructure/board.cpp`
- `src/infrastructure/main.cpp`
- `src/infrastructure/simulator.cpp`
- `tests/core_tests.cpp`
- `tests/test_architecture.py`
