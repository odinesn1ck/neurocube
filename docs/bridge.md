# Hardware Bridge NeuroCube

Bridge — единственный владелец CDC и операций обновления для выбранного серийного номера. Межпроцессная файловая блокировка автоматически освобождается ОС после завершения процесса. Прямое открытие порта сторонним терминалом вне Bridge этой блокировкой не контролируется; Windows не допускает совместное открытие CDC.

## Команды

```powershell
python tools/neurocube.py detect
python tools/neurocube.py --serial 419CB819454EA93D info
python tools/neurocube.py logs --seconds 20
python tools/neurocube.py records
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
NC1 selftest
NC1 screenshot
```

Ответы — JSON Lines с `v:1` и `type`. hello возвращает product, firmware, serial, capabilities, debug_input. Заводская прошивка не обязана поддерживать эти запросы. Несовместимый ответ или тайм-аут означает unsupported/ошибку, а не успешную диагностику.

status содержит экран, игру, счёт, номер записи, состояние сохранения, IMU и счётчики аппаратных событий/ошибок. imu пока возвращает ту же структуру status. Координаты ускорения — g, скорость Z — градусы/с. records возвращает 48 значений рекордов (16 зарезервированных слотов × 3 сложности; используются первые три игровых слота) в порядке игра × сложность и sequence. Экран: 0 Menu, 1 Rules, 2 Playing, 3 Paused, 4 Result, 5 Settings.

screenshot возвращает frame_begin, 240 frame_row с y и RGB565 big-endian hex, затем frame_end. Во время передачи игровой цикл блокируется; эта диагностическая функция не предназначена для работы во время активной игры.

Ограничение запроса — 159 символов; слишком длинная строка возвращает line_too_long. Неизвестный запрос — unsupported. Прямых команд питания, сброса и записи рекорда в production-протоколе нет. В debug-сборке дополнительно доступны inject, debug и release_input; они применяют синтетический InputFrame и должны явно маркироваться как синтетические проверки.

## Безопасное обновление

Сначала проверяется UF2, затем блокируется устройство и фиксируется его PnP-идентичность. После программного BOOTSEL проверяются VID/PID, USB LocationPath и единственность соответствия. При изменении serial используется только однозначное соответствие на прежнем USB-порту. picotool получает конкретный boot serial, проверяет запись, перезапускает плату; Bridge ждёт runtime hello с исходным serial. При любой неоднозначности запись не начинается.

Уже находящееся в BOOTSEL устройство принимается только при точном совпадении ожидаемого serial. Другой экземпляр не выбирается автоматически. Для нового устройства явно передайте --serial и отдельно подтвердите аппаратную модель и разрешение на прошивку.
