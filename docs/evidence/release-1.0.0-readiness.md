# Подготовка выпуска 1.0.0 — 22.10.2026

**Статус: подготовка, стабильный выпуск не опубликован.** Функции заморожены на 0.3.5. Source commit, итоговый manifest/SHA256, независимые сборки, память и CI будут добавлены после фиксированного кандидатного коммита.

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

Исторические физические подтверждения аккумулятора/PWR/звука не объявляются приёмкой новой версии. На плате остаётся 0.3.5 до отдельного разрешения установки. [Короткий физический чек-лист](../morning-checklist.md).

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
