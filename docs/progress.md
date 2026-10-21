# Состояние NeuroCube

Обновлено 21.10.2026. Ветка `codex/neurocube-mvp`, [PR #1](https://github.com/odinesn1ck/neurocube/pull/1), без merge. На плате `419CB819454EA93D` установлена **production 0.3.2**, исходный коммит `8edc20c35597`; debug input отключён. Предыдущие аппаратные свидетельства ниже относятся к указанным 0.3.0/0.3.1.

## Обратная связь: 0.3.2

Реализованы разнообразные первые сигналы Simon и полный текст правил, сенсорный поворот/допуски смешанных жестов Commands, shake-start Magic Ball. Native suite и 14 Python tests пройдены; правила полностью помещаются в кадр. [Отчёт](evidence/player-feedback-2026-10-21.md). Установлена на serial 419CB819454EA93D с readback verify, [USB smoke](evidence/feedback-usb-smoke-2026-10-21.json) пройден. Полностью сохранены sequence 16, рекорды Simon 5 / legacy maze 885 / Commands 5 и настройки/калибровка; BAT_EN HIGH. [Релиз 0.3.2-rc1](https://github.com/odinesn1ck/neurocube/releases/tag/v0.3.2-rc1), UF2 408576 байт, SHA256 `d4294a74ce8a2f8ed14d3e6141d1d38335c642a8346fb12ff6fe75219ca45d95`; скачанный файл побайтно совпал с установленным. Физическое удобство новых жестов остаётся NOT_TESTED.

## Исправления ревью 0.3.1

Реализованы процедурная генерация/повтор, непрерывное управление Commands, чувствительность, контакты, пагинация и устранение повторного gpio_init BAT_EN. VERIFIED_IN_SIMULATOR: 6000 seed (1932/2000/2000 разных карт по сложности), проверка всех проходов радиусом шара, 300 полных прохождений игровой физикой, 14 Python-тестов. Галерея подтверждает одинаковый кадр retry и другой new maze; [отчёт](evidence/review-fixes-2026-10-21.md). Установлен на плату с readback verify; [USB smoke](evidence/review-usb-smoke-2026-10-21.json) пройден: IMU/touch identity, кадры, I²C без ошибок, BAT_EN HIGH. До/после равны все рекорды и настройки: sequence 15, Simon 3 / legacy maze 885 / Commands 5, sensitivity 1.0, калибровка сохранена. [Релиз 0.3.1-rc1](https://github.com/odinesn1ck/neurocube/releases/tag/v0.3.1-rc1), UF2 407040 байт, SHA256 `531e5045eaebd57d381710798f73857206c65eb8f12c7da0b18f4e5f9f1bbb2b`. Опубликованный UF2 скачан и побайтно совпал; push/PR CI успешны. [Verification](evidence/review-verification-2026-10-21.json). Физические PWR/автономность и UI ещё NOT_TESTED; ниже сохранены свидетельства предыдущей поставки.

## Реализация предыдущей 0.3.0

Ранний BAT_EN HIGH; сохраняемая калибровка и чувствительность; правила/отсчёт/фазы Simon и Commands; девять проходимых карт с лимитами контактов и независимыми рекордами; 24 согласованные пары Magic Ball; «Привет, Андрей» → QR GitHub → меню; три перспективных IT-эпизода, 120 с idle только в Menu/Result, плавное гашение и поглощение первого wake-события. Подробное поведение — в [player-guide](player-guide.md), продуктовые решения — в [vision](product-vision.md).

Игры не зависят от Pico SDK. Подсветкой GPIO13 управляет отдельный адаптер; polling idle сохраняет работу USB/сенсора/PLUS и BAT_EN. Deep sleep и управление зарядкой не используются. [Архитектура idle](screensaver.md).

## Проверки предыдущей поставки 0.3.0

| Статус | Результат и доказательство |
|---|---|
| VERIFIED_IN_SIMULATOR | 1529 C++ проверок: фазы/отсчёт/рестарт, калибровка, миграция v1/v2/v3, прохождение всех 9 карт, контакты/лимит, 50000 выборов Magic Ball, layout 24 пар, idle/wake/возврат результата и границы трёх эпизодов. [Native evidence](evidence/star-crawl-native-2026-10-21.json) |
| VERIFIED_IN_SIMULATOR | 13 Python проверок Bridge/архитектуры; независимое декодирование QR с framebuffer в точный URL проекта. [Галерея](images/star-crawl-simulator-gallery.png) |
| VERIFIED_ON_HARDWARE | Пользователь подтвердил **5+ минут без USB** на минимальном power-fix `f98cf63`. [Первичное подтверждение](evidence/battery-acceptance-2026-10-21.md) |
| VERIFIED_ON_HARDWARE | Финальная 0.3.0: BOOTSEL по точному serial, load/readback verify, production hello, IMU/touch ID и samples, отсутствие новых I²C ошибок, BAT_EN HIGH. [USB smoke](evidence/final-usb-smoke-2026-10-21.json) |
| VERIFIED_ON_HARDWARE | Реальная запись v3 в неактивный сектор и чтение после перезапуска: sequence 11 → 12, все очки/настройки сохранены. Production-команда миграции возвращает unsupported. [Migration evidence](evidence/record-migration-2026-10-21.json) |
| VERIFIED_ON_HARDWARE | На окончательном `bfa1a73` наблюдались Crawl → затухание PWM → 0, остановка кадров, живой USB, BAT_EN HIGH и неизменные очки. [Финальное наблюдение](evidence/final-idle-probe-2026-10-21.json). Во время титров около 20 отправленных кадров/с; физические wake-события не зарегистрированы |
| COMPILED_ONLY | Debug-вариант также собран; использован только для совместимой записи журнала, затем заменён production. Воспроизводимость и стек подтверждены отдельно, это не физическая приёмка |
| NOT_TESTED | Физический touch/PLUS wake и видимое качество новых экранов человеком, отдельный холодный запуск PWR, энергопотребление, автономный sleep/wake на окончательном образе |

Ранее пользователь принял дисплей/меню, сенсор, паузу, shake, базовые игры, PLUS и реальные рекорды на предыдущей версии. Это сохранено в исторических наблюдениях; не выдаётся за полную приёмку всех новых экранов. Открытые критерии — в [backlog](backlog.md) и [чек-листе](morning-checklist.md).

## Сборка, память и доставка предыдущей 0.3.0

Финальный UF2: **403968 байт**, SHA256 `d59b63b199a135d73c83892117439238baa097632efd34aa5e1294fd01c4ec09`. Два независимых каталога дали одинаковый файл: [reproducibility](evidence/final-reproducibility-2026-10-21.json). Скачанный из GitHub UF2 также побайтно совпал с локальным. Комплект: [0.3.0-rc1](https://github.com/odinesn1ck/neurocube/releases/tag/v0.3.0-rc1), включая Windows-симулятор, лицензии и verification.json.

Проверка ARM-бинарника выявила недостаточный стандартный резерв стека для расширенного журнала. Постоянные App/CDC/FlashStore-буферы перенесены в .bss, core0 явно резервирует 4096 байт. Известная цепочка записи занимает около 2832 байт прикладных кадров; это не измерение полного IRQ/high-water. Реальная миграция успешно проверена. [Аудит стека](evidence/stack-audit-2026-10-21.json).

CI: **успешно**. После обновления PAT опубликован [workflow](../.github/workflows/build.yml); реальные [push](https://github.com/odinesn1ck/neurocube/actions/runs/39579001547) и [PR](https://github.com/odinesn1ck/neurocube/actions/runs/39579001945) прогоны на Windows прошли проверку документации, C++/13 Python-тестов, headless-симулятора и сборку RP2350. Оба архива скачаны, SHA256 архивов и UF2 проверены. [Доказательства](evidence/ci-2026-10-21.json). Эти CI-образы не прошивались; аппаратный статус выше относится к bfa1a73.

## Сохранность и документация

Реальные Normal-очки: Simon 3, legacy maze 885, Commands 1. Они сохранены; новые карты не сравниваются с legacy maze. До изменений сохранены оба сектора (8192 байта) вне git: `%USERPROFILE%/neurocube-tools/backups/records-before-polish-2026-10-21.bin`, SHA256 `73d92ee32cab5560bdafb9c7315199bf876d5966edf805bf3e51d208ff11c361`. Заводская копия не изменялась; сведения в [hardware](hardware.md).

Документальный этап `fd13e96` завершён отдельно: политика, AGENTS, источники истины, backlog/changelog/progress, навигация и проверка ссылок. Исходный код и flash в этом этапе не менялись. [Полный аудит и список файлов](evidence/documentation-audit-2026-10-21.md). Последующее изменение QR/idle внесено по новому явному поручению пользователя; словарь Magic Ball и каталог трёх сетов сохранены.

## Уточнение подсказки Magic Ball

Статическое ревью `3c9349b` записано в [отчёте](evidence/code-review-2026-10-21.md): тесты и плата в том проходе не запускались. Открытые расхождения — в [backlog](backlog.md) (GAME-003a, GAME-002a, POWER-001a, UX-003).

По новому указанию пользователя подсказка на игровом экране и в кратком описании заменена точной строкой «Задумай ответ, встряхни шар». VERIFIED_IN_SIMULATOR: 1529 существующих C++ проверок пройдены, [кадр 240×240](images/magic-hint-2026-10-21.png) просмотрен — текст полностью помещается. Механика shake/touch/«Спросить ещё», задержки и пары ответов не менялись. На этапе изменения текста плата оставалась на bfa1a73; впоследствии подсказка вошла в установленную 0.3.1 (см. выше). UF2 новой версии собирается workflow, статус и артефакт доступны в [Actions](https://github.com/odinesn1ck/neurocube/actions).
