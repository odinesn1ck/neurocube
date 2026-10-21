# Hardware Bridge NeuroCube

## Диагностика интерфейса 0.3.3

Read-only `NC1 ui`: JSON type=ui, screen, idle_phase, progress, brightness, maze_seed. Фазы idle: 0 Awake, 1 Crawl, 2 Fading, 3 Dark. Screen 13 — ShuttingDown, 14 — PowerNotice. Команда не переводит устройство в другой режим и не меняет рекорды. Для фактического PWM используйте `NC1 display`; видимое изображение подтверждается отдельно человеком.


Bridge — единственный владелец CDC и операций обновления для выбранного серийного номера. Межпроцессная файловая блокировка автоматически освобождается ОС после завершения процесса. Прямое открытие порта сторонним терминалом вне Bridge этой блокировкой не контролируется; Windows не допускает совместное открытие CDC.

## Команды

```powershell
python tools/neurocube.py detect
python tools/neurocube.py --serial 419CB819454EA93D info
python tools/neurocube.py logs --seconds 20
python tools/neurocube.py records
python tools/neurocube.py power
python tools/neurocube.py settings
python tools/neurocube.py display
python tools/neurocube.py selftest
python tools/neurocube.py screenshot artifacts/frame.png
python tools/neurocube.py build --native
python tools/neurocube.py build
python tools/neurocube.py flash build-pico/neurocube.uf2 --allow-flash
```

Используйте Python из установленного venv. detect читает PnP и список COM без открытия порта. logs выполняет handshake и опрашивает status, переподключаясь в пределах заданного времени. Порт определяется по VID/PID и serial, а не по COM3. Отсутствие или неоднозначность соответствия — ошибка.

Скриншот — framebuffer прошивки, не фотография LCD. selftest проверяет электронный обмен и честно возвращает NOT_TESTED для физического ввода, изображения и аккумулятора.

## Протокол версии 1

USB CDC 115200, RTS=false, DTR=true. Текстовые запросы ASCII, окончание LF:

```text
NC1 hello
NC1 status
NC1 imu
NC1 records
NC1 power
NC1 settings
NC1 display
NC1 selftest
NC1 screenshot
```

Ответы — JSON Lines с `v:1` и `type`. hello возвращает product, firmware, revision, serial, capabilities, debug_input. Заводская прошивка не обязана поддерживать эти запросы. Несовместимый ответ или тайм-аут означает unsupported/ошибку, а не успешную диагностику.

status содержит экран, игру, счёт, номер записи, состояние сохранения, IMU и счётчики аппаратных событий/ошибок. imu пока возвращает ту же структуру status. Координаты ускорения — g, скорость Z — градусы/с. records возвращает 48 значений рекордов (16 зарезервированных слотов × 3 сложности; используются первые три игровых слота) в порядке игра × сложность и sequence. В v3 добавлены `maze_best` (геометрия × вариант × лимит, 54 значения) и `record_version`; legacy `best` не меняет порядок. Экран: 0 Menu, 1 Rules, 2 Playing, 3 Paused, 4 Result, 5 Settings, 6 Calibration, 7 Countdown, 8 MazeSetup, 9 Splash, 10 ProjectQr, 11 Crawl (включая затухание), 12 DisplayIdle.

`power` читает BAT_EN, KEY_PWR, raw STAT/ADC, не меняет выходы; `settings` возвращает сохранённую калибровку и чувствительность. Напряжение и процент заряда не вычисляются.

screenshot возвращает frame_begin, 240 frame_row с y и RGB565 big-endian hex, затем frame_end. Во время передачи игровой цикл блокируется; эта диагностическая функция не предназначена для работы во время активной игры.

Ограничение запроса — 159 символов; слишком длинная строка возвращает line_too_long. Неизвестный запрос — unsupported. Прямых команд питания, сброса и записи рекорда в production-протоколе нет. В debug-сборке дополнительно доступны inject, debug и release_input; они применяют синтетический InputFrame и должны явно маркироваться как синтетические проверки.

## Безопасное обновление

Сначала проверяется UF2, затем блокируется устройство и фиксируется его PnP-идентичность. После программного BOOTSEL проверяются VID/PID, USB LocationPath и единственность соответствия. При изменении serial используется только однозначное соответствие на прежнем USB-порту. picotool получает конкретный boot serial, проверяет запись, перезапускает плату; Bridge ждёт runtime hello с исходным serial. При любой неоднозначности запись не начинается.

Уже находящееся в BOOTSEL устройство принимается только при точном совпадении ожидаемого serial. Другой экземпляр не выбирается автоматически. Для нового устройства явно передайте --serial и отдельно подтвердите аппаратную модель и разрешение на прошивку.

## Синтетическая интеграционная проверка

Исторический сценарий `tools/hardware_smoke.py --allow-synthetic-and-reboot --clear-generated-records` рассчитан на старые экраны 0.1.x и пустые рекорды; для polish 0.2.x не подходит. Не запускать на пользовательских рекордах. Он работает только с отдельно установленной debug-сборкой. Требует изначально пустую таблицу рекордов перед разрешением очистки, проверяет навигацию, по одному успешному действию Sequence/Commands, запуск Maze, оба способа запроса Magic Ball и сохранение после настоящего перезапуска. Успешный прогон удаляет только созданные тестовые результаты. Это не физическое прохождение игр. После прерванного прогона сначала проверьте report и рекорды; не очищайте неизвестные результаты.

Отчёт создаётся в artifacts/hardware-synthetic-smoke.json. На production-прошивке inject, debug, release_input и clear_test_records возвращают unsupported.

## Production smoke без изменения данных

`python tools/polish_smoke.py --baseline records-before.json` проверяет hello, disabled debug input, питание, настройки, электронный selftest, движение кадров, ошибки I²C и неизменность рекордов. Baseline — сохранённый JSON-ответ `NC1 records`; проверка сравнивает реальные очки. Не посылает inject, reset, BOOTSEL или команды записи. Не запускать screenshot во время физического прохождения: передача временно останавливает игровой цикл. Батарея и физическое управление остаются NOT_TESTED без ручного результата.

`display` возвращает запрошенную яркость, PWM-регистр и выбранный polling idle. В Dark счётчик кадров останавливается, USB и ввод продолжаются; production smoke учитывает это. Физическое пробуждение не выводится из значения регистра.

Для явной проверки миграции существует только в debug-сборке `NC1 migrate_records`: в главном меню копирует текущие очки/настройки без изменения значений, увеличивает sequence и пишет совместимый v3 в неактивный сектор с readback. Перед использованием сохраняют baseline/backup; после команды требуется перезапуск для чтения нового журнала. Production возвращает unsupported. Это проверка Flash, не игровой ввод и не физическая приёмка управления.

`python tools/idle_probe.py --seconds 185` только наблюдает production status/display/power/records и фиксирует переходы и реальные счётчики событий. Он не вводит touch/кнопку и не сбрасывает idle-таймер. Нулевое число wake-событий означает отсутствие подтверждения, а не успешную проверку пробуждения.

Read-only `NC1 audio` (CLI `python tools/neurocube.py audio`, 0.3.4): enabled, ready, busy, played, dropped, faults, pcm_peak. Команда не воспроизводит звук. `NC1 settings` дополнен sound_enabled; старые поля не меняются.
