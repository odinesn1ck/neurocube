# Замена ES8311: происхождение, эквивалентность и приёмка

> Актуальный итог: **1.0.0 опубликована** по поручению владельца из принятого f6c60d7/PCM2560; пользователь подтвердил «звук ок». [Публикация и обратная проверка всех assets](release-1.0.0-published.json), [текущий прогресс](../progress.md). Current-code license PASS; история прежнего restricted распространения остаётся открытой LICENSE-002. Ниже сохранены этапы и прежние статусы времени подготовки, включая NOT_TESTED/BLOCKED до приёмки; они не являются текущим статусом выпущенного snapshot. Полный новый физический прогон всех функций не заявляется.

## Текущий статус

Реализована узкая замена спорного драйвера на Apache-2.0 derivative. Право использования нового кода на RP2350 подтверждено явной лицензией официального компонента; аппаратная аудиоприёмка новой реализации **NOT_TESTED**. Стабильный выпуск остаётся BLOCKED до проверки и решения по историческим публикациям. Предварительная регрессия PASS: 4 native suite, 18 Python tests, Windows simulator GUI/headless/gallery, ARM Release. [Сводка](es8311-replacement-tests.json). Воспроизводимая поставка/CI фиксируются после source commit.

## Подтверждение прав

Источник — [Espressif esp-bsp, закреплённый компонент](https://github.com/espressif/esp-bsp/tree/f259a6abd5808019b1903a422092a4234ca95a51/components/es8311). `es8311.c`: SPDX copyright 2015–2026 Espressif Systems (Shanghai) CO LTD, Apache-2.0; `es8311_reg.h`: 2015–2021 Espressif, Apache-2.0. Компонент имеет собственный полный LICENSE Apache-2.0, файл NOTICE в его каталоге отсутствует. [SHA256 каждого исследованного исходника](../../third_party/espressif_es8311/provenance.json).

История официального файла проверена до первоначального `7889f2ad923b` (Add BSP components): уже там SPDX Apache-2.0 и отдельный Apache LICENSE. Изменения e963a6719aaa (MCLK), 1542d0521ed8 (error checking), последующие форматирования сохраняют лицензию. Совпадения стандартных названий регистров/алгоритмов с legacy-кодом не являются нашим источником заимствования: производный текст выбран только из закреплённого Apache esp-bsp, регистровый header скопирован из него без изменений. Это документированная адаптация разрешённого источника, не заявление о clean-room разработке: старый код изучался для карты используемых интерфейсов, но не копировался.

Apache-2.0 §§2–4 разрешает воспроизведение, производные произведения и распространение Source/Object, не ограничивая вендора CPU. Поэтому RP2350 допустим при выполнении условий: передать текст лицензии, сохранить attribution/copyright, обозначить модификации и сохранить применимый NOTICE, если он есть. Собственная MIT NeuroCube не перелицензирует этот компонент. §3 содержит патентные условия, §6 не предоставляет товарные знаки. Основание — [официальный текст Apache](https://www.apache.org/licenses/LICENSE-2.0) и лицензия самого компонента, а не предположение по корню Waveshare. Адаптация переносит только I²C/таймерные callbacks; ESP-IDF не включается. Deprecated-статус upstream отмечен: версия закреплена и тестируется, никакого автоматического обновления на esp_codec_dev нет.

## Карта используемых операций

| Назначение | Прежний вызов | Новая реализация / источник |
|---|---|---|
| ID до включения усилителя | Es8311_Read_Id | es8311_read_id, FD/FE из Apache register header; принимает те же 8311/1183 |
| Reset / init | Es8311_Init | Apache init: 00 reset, 20 ms, clock config, format, analog/DAC power |
| Частота | Sample_Frequency_Config через init | Только coefficient 6144000/24000: 02 prediv, 03/04 OSR=10, 05 ADC/DAC div=0, 06 BCLK div=3, 07/08 LRCK=00FF |
| Master / формат | Fmt_Config | 00 bit6=1, 09/0A=0C (I²S16); Apache fmt изменён только для master, hardware fact из ES8311 User Guide §8.2 |
| DAC/power/filter | init | 0D=01, 0E=02, 12=00, 13=10, 1C=6A, 37=08 из Apache init |
| Цифровое усиление | Voice_Volume_Set(60) | Apache mapping: 32=98; clamp 0..100; PCM peak 1280 не меняется |
| Mute/unmute | Voice_Mute | Apache read-modify-write 31 bits5/6; остальные биты сохранены |
| Ошибка I²C | общий latch codec_io_ok | callback status + немедленный return; HAL оставляет PA выключенным при init error |
| Завершение / stop | HAL DMA/PA sequencing | Без изменения: GPIO0 off, нули/cooldown, DAC mute, остановка MCLK; нет новых codec shutdown/GPIO команд |

У кодека нет владения BAT_EN/зарядкой/кнопками. Неизменны PIO/DMA, GPIO0/1/3/4/5, I²C1 6/7 address18, MCLK6.144MHz, 24kHz, volume60, PCM1280, preparation180ms/release120ms. [User Guide §8.2](https://files.waveshare.com/wiki/common/ES8311.user.Guide.pdf#page=13) используется как источник факта о bit6, PDF с пометкой Confidential не включается в поставку; текст/таблицы из него не копируются. Игры, events, mute, приветствие/выключение, журнал v3, калибровка и настройки не изменяются.

## Состав и проверка

Старые es8311.c/.h и шины DEV_Config/audio_pio.h удалены из HEAD. В CMake единственный codec source — `third_party/espressif_es8311/es8311.c`. Старый standalone RAM probe выведен из использования: он подтягивал спорный внешний драйвер и громкий PCM8191. Первичные исторические отчёты сохранены. Builder запрещает возвращение старых исходников и их упаковку; включает новый LICENSE, provenance, change notice. Freeze всех остальных core/header/board/display файлов проверяется против v0.3.5-rc1, исключение audio.cpp закреплено SHA256 reviewed adaptation.

Нативный codec_tests выполняет реальный C-драйвер с fake I²C: конечные clock/master/format/gain регистры, сохранение остальных битов, delay20ms, отказ на каждой init transaction, mute/ID/volume failures. Audio/core/timing suite проверяет прежние игровые события, debounce, one-shot, mute/persistence, shutdown и 32 раунда. Отдельные Python-тесты защищают provenance и отсутствие старых файлов. Эти проверки не доказывают слышимость или отсутствие щелчков на реальной плате.

## Исторические Git objects и assets

Новый Apache grant относится к другому конкретному официальному произведению и нашей адаптации; он **не является ретроактивным разрешением** старого restricted файла. История и старые release assets по-прежнему содержат старый драйвер/собранный код. Нельзя назвать весь публичный history лицензионно очищенным только после удаления файла в HEAD.

План владельцу: получить у правообладателя ясное разрешение для прежней публикации либо согласовать отдельный план прекращения распространения прежних исходников/бинарников и, при необходимости, удаления старых assets/очистки истории с учётом форков и кэшей. Новые чистые source/firmware archives можно оценивать отдельно от старых публикаций; удаление не заменяет ретроактивное разрешение. Агент не отправляет письма, не удаляет assets, не переписывает history, не выполняет force push и не меняет visibility. Перед объявлением общей Public Readiness требуется решение владельца по этому риску. Аппаратная проверка не решает вопросы исторического распространения.

## Безопасная аппаратная приёмка и rollback

До записи — отдельное разрешение конкретного кандидата, проверка serial/topology, новый backup8192 байта вне Git, picotool readback verify, сравнение records/settings. До установки Apache-кандидата на устройстве была 1.0.0/89007ce с прежним драйвером. Rollback image с SHA256 `8fee590574092ccf9ab33753d43d739ef63a09d686a7a7f97061a5d1e0128f8a` сохранён вне новых release archives; не переопубликовывается как лицензионно чистый. При неисправности остановить звук и согласовать восстановление этой локальной проверенной версии с теми же backup/verify guards.

После разрешённой записи пользователь проверяет комфорт «уоп» приветствия/Magic Ball/успеха и «кх» ошибки/стены/выключения, общий mute (системные сигналы звучат всегда), отсутствие щелчков/пульсаций. Только принятый PCM1280; громкий диагностический тон не используется. Проверить PWR, автономность и сохранность записей после включения. Физическое звучание до подтверждения человеком остаётся **NOT_TESTED**; электронный результат разрешённой установки приведён ниже.

## Проверка упаковки между компьютером и CI

Первый Apache-кандидат 0956f4f прошёл CI; скачанные UF2/ELF/BIN/Windows ZIP/full ZIP/manifest совпали побайтно. Отдельный source ZIP имел одинаковые файлы, но Git записал время ZIP entries в локальном TZ вместо UTC. Builder исправлен: source ZIP использует ту же UTC-упаковку canonical Git-байтов, что и остальные архивы. Для окончательного поставочного кандидата проверка повторяется; это изменение только упаковки, без изменений codec или игр.

## Окончательный проверенный кандидат

Source commit: `fb6d178a131bff350b5523add7d37fa746f72f91`. [Полная проверка поставки](es8311-replacement-candidate.json). [CI push PASS](https://github.com/odinesn1ck/neurocube/actions/runs/39673975815), [CI PR PASS](https://github.com/odinesn1ck/neurocube/actions/runs/39673982172). [Скачать все артефакты](https://github.com/odinesn1ck/neurocube/actions/runs/39673975815/artifacts/12380739205).

UF2: 518656 байт / 1013 блоков; SHA256 `96a7f6103e34ce7cf8ad628e9fe493d7c423ed36566abfdbedff1ddd0551ffa3`. Локальные файлы: `artifacts/release-1.0.0-apache-final/dist` (UF2/ELF/BIN, Windows/source/full ZIP, manifest, SHA256SUMS.txt). Две независимые сборки и повторная упаковка совпали. После UTC-исправления **все** скачанные CI dist-файлы, включая source ZIP и SHA256SUMS.txt, побайтно совпали с локальными.

Статические guards: 27 frozen sources без изменений относительно v0.3.5-rc1; adapter hash соответствует provenance; debug injection OFF; UF2 не пересекает journal/reserved page. CRC/manifest всех ZIP PASS; отсутствие старых C/header проверено по именам, SHA256 исходных файлов и restricted grant в C/header contents. Новый LICENSE/provenance включены во все пакеты.

Регрессия: 100640959 core, 2986825 timing, 10746 audio, 2503 codec checks; 4 native suite, 18 Python tests, Windows GUI/headless/gallery PASS. Основная статическая RAM 181376 байт; до main RAM limit остаётся 342912, core0 stack4096. Крупнейший измеренный core frame880; это не динамический high-water. Относительно прежнего кандидата основная RAM увеличилась на 16 байт из-за callback transport; игровой буфер/звуковой синтез не менялись.

Аудит source commit: 1059 доступных Git-объектов, tracked worktree, 9 прежних релизов/72 assets; новые ZIP отдельно проверены. Known PAT in-memory и Gitleaks history/worktree/assets/candidate: PASS, credential findings0. 41 прежняя privacy location: 28 generic developer build paths в старых материалах и 13 обязательных copyright-контактов. Новые production binary paths очищены. Это проверка доступного снимка с ограниченными детекторами, не математическое доказательство отсутствия любых секретов; старые license issues проверка секретов не снимает.

Итог: **current-code license PASS**, **software/CI/reproducibility PASS**, **replacement hardware acceptance NOT_TESTED**, **historical distribution unresolved**, **stable release BLOCKED**. После отдельного разрешения пользователя окончательный fb6d178 установлен; подробности ниже. В раннем запросе фигурировал 0956f4f; он заменён запросом на fb6d178 после исправления source ZIP. Стабильный release, tag, merge, history rewrite и удаление старых assets не выполнялись.

## Разрешённая установка fb6d178

Пользователь: «грузи в железо». [Отчёт](es8311-replacement-flash.json). Проверены serial419CB819454EA93D и прежняя USB topology; перед записью сохранён и readback-верифицирован свежий журнал8192 байта вне Git, найдена текущая v3 страница с корректным CRC/sequence. picotool load/readback PASS; runtime1.0.0/fb6d178, debug_input=false. Все records и settings до/после равны. IMU/touch/LCD electronic selftest PASS, BAT_EN HIGH.

Audio telemetry: ready=true, played=1 (приветствие), dropped=0, faults=0, PCM1280. После завершения сигнала busy=false и faults=0. Это **VERIFIED_ON_HARDWARE для USB/I²C/завершения аудиопередачи**, не подтверждение акустики или автономности. Пользователю направлен запрос проверки комфортных «уоп»/«кх» без щелчков/пульсаций. До ответа физическая аудиоприёмка NOT_TESTED. Старые assets/history, merge и stable release не изменялись.
