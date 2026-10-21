# Реестр доказательств и отчётов

Каждое свидетельство относится к указанной версии и способу проверки. Сводка текущего состояния: [progress](../progress.md); задачи: [backlog](../backlog.md).

| Свидетельство | Что подтверждает |
|---|---|
| [Исходная USB-диагностика](../hardware-diagnostics-2026-10-21.md) | Историческое состояние до разработки |
| [Отчёт первого MVP](../overnight-results-2026-10-21.md) | Ночной прототип, прежний релиз |
| [Первый synthetic smoke](hardware-synthetic-smoke.json), [production telemetry](hardware-final.json), [reproducibility](reproducibility.json) | Предыдущая 0.1.0; синтетический ввод не равен физическому |
| [Сырые данные power-fix](power-first-2026-10-21.json) | GPIO28 HIGH и сохранность очков |
| [Физическая приёмка аккумулятора](battery-acceptance-2026-10-21.md) | Сообщение пользователя о 5+ минутах без USB |
| [Программная проверка polish](polish-validation-2026-10-21.json) | Native-тесты, карты, пары Magic Ball и ограничение CI |
| [Polish USB smoke](polish-usb-smoke-2026-10-21.json) | Установленная 0.2.0, электроника и реальные рекорды |
| [Polish reproducibility](polish-reproducibility-2026-10-21.json) | Два одинаковых UF2 из указанного коммита |
| [Галерея симулятора](../images/polish-simulator-gallery.png) | Программные кадры, не фотографии LCD |
| [Архив аппаратных заметок](hardware-notes-before-governance-2026-10-21.md) | Исходные факты до разделения документации |
| [Аудит документации](documentation-audit-2026-10-21.md) | Найденные противоречия и результат проверки ссылок |

## Скринсейвер 0.3.0

- [Native и QR decode](star-crawl-native-2026-10-21.json).
- [Production USB smoke](star-usb-smoke-2026-10-21.json).
- [Два идентичных UF2](star-reproducibility-2026-10-21.json).
- [Кадры симулятора](../images/star-crawl-simulator-gallery.png).

## Финальная поставка 0.3.0-rc1

- [Production USB smoke](final-usb-smoke-2026-10-21.json), [реальная миграция журнала](record-migration-2026-10-21.json).
- [Воспроизводимость финального UF2](final-reproducibility-2026-10-21.json), [бюджет ARM-стека](stack-audit-2026-10-21.json).
- [Первый аппаратный idle-прогон](idle-probe-2026-10-21.json): PWM/USB, без физического wake.
- [Итоговый отчёт](delivery-2026-10-21.md).

- [Естественный idle на финальном bfa1a73](final-idle-probe-2026-10-21.json): PWM 0, USB жив, рекорды сохранены; physical wake не зарегистрирован.

## Ревью исходников

- [Статическое ревью ветки](code-review-2026-10-21.md): чтение `3c9349b`, без повторного прогона тестов и без прошивки. Аппаратную приёмку не заменяет.

## GitHub Actions

- [CI после обновления PAT](ci-2026-10-21.json): успешные push/PR-запуски, состав и SHA256 скачанных артефактов. Аппаратных операций не было.

- [Исправления статического ревью и процедурные карты](review-fixes-2026-10-21.md).

- [USB smoke 0.3.1](review-usb-smoke-2026-10-21.json), [CI/доставка 0.3.1](review-verification-2026-10-21.json).

- [Обратная связь по играм 0.3.2](player-feedback-2026-10-21.md).

- [USB smoke 0.3.2](feedback-usb-smoke-2026-10-21.json).
