# Подготовка выпуска 1.0.0 — 22.10.2026

**Статус: подготовка, стабильный выпуск не опубликован.** Функции заморожены на 0.3.5. Кандидат собран из `89007ced036608535bf3ed465b22d83c428ad9bd`. Полный результат: [машиночитаемая проверка](final-1.0.0-candidate.json).

## Обновление: замена codec driver

[Apache-2.0 замена ES8311](es8311-license-replacement.md) имеет подтверждённое право использования на RP2350. Сборка 89007ce и её установка ниже — прежний кандидат со старым драйвером. Его хеш не является хешем новой поставки. Новая аппаратная аудиоприёмка и решение по историческим публикациям пока открыты; stable release BLOCKED.

## Регрессия

Регрессия замороженного ядра с финальными build-флагами: 100640959 core checks, 2986825 Simon timing checks, 10746 audio checks, 14 Python tests, headless и галерея Windows simulator пройдены.

| Область | Программная проверка | Физическая 1.0 |
|---|---|---|
| Simon 32 раунда, скорость, Recall/restart | sequence_timing_tests + core | NOT_TESTED |
| Maze seed/retry/new/проходимость/контакты | 6000 seed, 300 прохождений физической моделью | NOT_TESTED |
| Commands swipe/gyro/touch, приоритет ввода | review_input/player_feedback/gesture tests | NOT_TESTED |
| Magic Ball 24 пары, shake, QR/splash | polish/games/framebuffer + gallery | NOT_TESTED |
| Mute, one-shot contact/success, persistence | audio_tests + legacy v1/v2/v3 | NOT_TESTED |
| PWR/save/latch/ошибка записи | acceptance_followup + records/recovery | NOT_TESTED |
| Crawl/idle/wake, отсутствие IMU wake | idle tests + gallery | NOT_TESTED |

Исторические физические подтверждения аккумулятора/PWR/звука не объявляются приёмкой новой версии. После отдельного разрешения установлена 1.0.0; физическая приёмка остаётся открытой. [Короткий физический чек-лист](../morning-checklist.md).

## Публикация и лицензии

[MIT оригинального кода](../../LICENSE) и [third-party inventory](../../THIRD_PARTY_LICENSES.md) подготовлены. [Лицензионный аудит](licenses-1.0.0.md): **BLOCKED ES8311**, заменить его заголовок на обычный MIT нельзя. [Public Readiness](public-readiness-1.0.0.md): секреты не найдены, репозиторий уже public при начале проверки. Агент не менял visibility, не переписывал историю, не удалял старые assets и не выполнял merge.

## Действия владельца

1. Разрешить лицензионный блокер: подтверждение от правообладателя либо отдельная проверенная замена с планом для истории/прежних артефактов.
2. После готового комплекта — отдельно решить, создавать ли неопубликованный draft v1.0.0 и устанавливать ли кандидат с backup/verify.
3. Пройти физическую приёмку, затем явно подтвердить stable publication. CHANGELOG получит выпуск 1.0.0 только после фактической публикации.
4. Merge main — отдельное решение; public visibility уже установлена и здесь не изменялась.

Сборка/архив или draft не снимают BLOCKED. Подготовленные release notes: [заметки](../release-notes-1.0.0.md).

## Дополнительные выполненные проверки

[Native/Python summary](final-1.0.0-regression.json). [Windows GUI smoke](final-1.0.0-gui-summary.json): настоящее скрытое окно создано и закрыто через WM_CLOSE, exit 0; хранилище изолировано вне Git. [Read-only USB](final-1.0.0-usb-summary.json): установленная 0.3.5, debug=false, ID IMU/touch/LCD отправка, frame progress, I²C без новых ошибок, BAT_EN HIGH, records/settings неизменны, audio ready/faults=0. Это не физическая приёмка 1.0.

Уточнение количества аудиопроверок: ранний этап до ускорения Simon дал 10757, итоговый код после ускорения — 10746 (меньше итераций проверяемого цикла показа). Оба запуска успешны; для кандидата учитывается фактический LastTest.log, а не прежнее число из архива.

## Финальная поставка

Две независимые Release-сборки UF2/ELF/BIN/Windows EXE совпали побайтно; повторная упаковка ZIP также совпала. Сборка сравнила 28 замороженных исходников с v0.3.5-rc1, debug injection выключен. UF2: 521728 байт, 1019 блоков; reserved page и журнал не затронуты. SHA256:

`8fee590574092ccf9ab33753d43d739ef63a09d686a7a7f97061a5d1e0128f8a`

[GitHub Actions PASS](https://github.com/odinesn1ck/neurocube/actions/runs/39669714265). [Скачать CI-пакет с UF2, симулятором, исходниками, manifest и SHA256SUMS.txt](https://github.com/odinesn1ck/neurocube/actions/runs/39669714265/artifacts/12378454174). Скачанный UF2 побайтно совпал с локальным. Все три ZIP прошли CRC и сверку хешей manifest. Локальная поставка: `artifacts/release-1.0.0-final/dist`.

Статическая основная RAM: 181360 байт; до лимита основной RAM остаётся 342928 байт. Стек core0 — 4096 байт; крупнейший отдельный статический кадр — 880 байт. Анализ .su не измеряет полный динамический максимум стека, библиотечные кадры и вложенные прерывания. Новых игровых/HAL путей выполнения нет. В ELF/UF2/BIN/EXE нет локальных путей Users/git после нормализации toolchain prefix.

Gitleaks и точное сравнение известного PAT в памяти не обнаружили секретов в новых архивах. [Итог аудита](final-1.0.0-public-audit.json). Поставка технически подготовлена; лицензионный BLOCKED и физическая приёмка остаются открытыми. Tag, draft и stable release не выполнялись. Flash 1.0.0 впоследствии выполнен по отдельному разрешению пользователя; см. отчёт ниже.

## Установка по разрешению пользователя

[Отчёт flash 1.0.0](final-1.0.0-flash.json): serial/topology проверены, свежий журнал 8192 байта сохранён и верифицирован вне Git; picotool load readback PASS. Runtime сообщает 1.0.0 / 89007ce, debug_input=false. Рекорды и все настройки до/после совпали, BAT_EN HIGH, audio ready/faults=0, электронная самопроверка PASS. Физическая приёмка по-прежнему NOT_TESTED: USB не доказывает автономную работу, видимость LCD и слышимость звука. Лицензионный BLOCKED не снят установкой.

## Итог после Apache-замены

Кандидат **fb6d178**, UF2 SHA256 `96a7f6103e34ce7cf8ad628e9fe493d7c423ed36566abfdbedff1ddd0551ffa3`. [Поставка, CI и воспроизводимость](es8311-replacement-candidate.json), [источники и регистры](es8311-license-replacement.md). Все программные проверки PASS; downloaded CI dist полностью совпал с локальным, старого драйвера в HEAD/новых ZIP нет. Отчёты 89007ce выше остаются историей предыдущей сборки и её разрешённой установки.

Новый кандидат **не установлен**, hardware audio NOT_TESTED; до записи нужны отдельное разрешение, identity, свежий backup8192 вне Git и readback/records/settings comparison. Общая готовность stable BLOCKED: физическая аудиоприёмка замены, решение владельца по старым restricted публикациям и окончательное подтверждение выпуска.
