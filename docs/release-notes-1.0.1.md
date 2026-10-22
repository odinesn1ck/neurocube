# NeuroCube 1.0.1

Небольшое обновление экрана выключения после [1.0.0](https://github.com/odinesn1ck/neurocube/releases/tag/v1.0.0).

Под «Выключаюсь» добавлена надпись:

© 2026,
мальчик Коля + OpenAI

Знак © добавлен в существующий OFL-шрифт Noto Sans Mono. Все164 прежних глифа сохранены. Порядок удержания PWR, сохранения данных и отключения батареи, игры, PCM2560/codec gain60, mute, приветствие и QR не изменены. Пользователь подтвердил надпись на установленной плате: «надпись ок, отправь в релиз».

## Поставка и проверки

Source/tag: `0cf9d59a7c240f3c6b6672d2fc40842a5a6693ee` / `v1.0.1`. Публикуются точные установленные бинарники, без пересборки. UF2 SHA256:

`1cd9e20a4d7e7f8ac918e285d949d4491a893d27cee85d2abe753dfaff57baf8`

4 native suite, 18 Python tests, галерея симулятора и ARM Release PASS. Две независимые production-сборки UF2/ELF/BIN/Windows EXE и повторные ZIP совпали; весь CI/local dist побайтно идентичен. [CI](https://github.com/odinesn1ck/neurocube/actions/runs/39768065217).

Проверена плата419CB819454EA93D: перед flash сохранён свежий журнал8192 вне Git, picotool readback verify PASS, все records/settings до/после равны. Электронная самопроверка PASS; BAT_EN HIGH, audio ready/busy=false/faults0/PCM2560. Визуальная надпись на LCD принята пользователем; акустика PCM2560 ранее принята на1.0.0. Новый полный физический прогон остальных функций не заявляется.

В комплекте UF2/ELF/BIN, Windows simulator, source/full ZIP, лицензии/notices, руководство, manifest/SHA256, release-publication.json и снимок симулятора shutdown-screen.png (не фотография LCD). Перед обновлением проверить модель/идентичность, сохранить журнал, не применять erase-all.

Build manifest и документы внутри исходных ZIP отражают момент сборки до последующей физической приёмки. Пакеты сохранены неизменными для проверяемого соответствия CI; итоговая приёмка и выпуск отражены этими заметками и отдельным publication record. SHA256SUMS.txt расширен хешами дополнительных release files, бинарники и три ZIP совпадают с CI.

## Лицензии

Собственный код NeuroCube — MIT; текущий ES8311 — Apache-2.0 Espressif esp-bsp с подтверждённым разрешением на RP2350; шрифт — SIL OFL1.1. Все notices/provenance включены. Спорного legacy ES8311 нет в новых snapshot/архивах. Отдельная LICENSE-002 по старому restricted коду в прежней публичной истории/assets остаётся открытой; новый grant не ретроактивен. Старые релизы и история не изменялись. Merge main не выполнялся.

Опубликован: 2026-10-22T11:20:38Z. [Release](https://github.com/odinesn1ck/neurocube/releases/tag/v1.0.1). [Проверенные assets](evidence/release-1.0.1-published.json).
