# Состояние NeuroCube

## Последний стабильный выпуск 1.0.1

[NeuroCube1.0.1 опубликован](https://github.com/odinesn1ck/neurocube/releases/tag/v1.0.1) по подтверждению пользователя «надпись ок, отправь в релиз». Source/tag: `0cf9d59a7c240f3c6b6672d2fc40842a5a6693ee` / `v1.0.1`; точные установленные бинарники, без пересборки. Под «Выключаюсь» показывается «© 2026, мальчик Коля + OpenAI» в две строки; один новый OFL-глиф, прежние164 сохранены. Питание/сохранения/игры/PCM2560 не менялись.

4 native suite /18 Python tests /галерея/ARM/независимая воспроизводимость/CI PASS. Все11 assets скачаны обратно и побайтно сверены, UF2 дополнительно проверен по публичной ссылке после публикации. [Факт выпуска и SHA256](evidence/release-1.0.1-published.json). [Flash](evidence/shutdown-copyright-1.0.1-flash.json): свежий backup8192 вне Git, readback и records/settings comparison PASS, электронная диагностика без ошибок. Визуальная надпись на реальной плате принята пользователем; новый полный физический прогон остальных функций не заявляется.

1.0.0 сохранена неизменной, main не слит; LICENSE-002 по старым публикациям остаётся отдельной открытой задачей.

## Выпуск 1.0.0 — история

[NeuroCube 1.0.0 опубликован](https://github.com/odinesn1ck/neurocube/releases/tag/v1.0.0) по прямому поручению пользователя «звук ок, финализируй релиз». Source/tag: `f6c60d749babd2295ffb1106ef7c6426618cf9a8` / `v1.0.0`. Выпущены точные принятые и установленные бинарники, без пересборки. [Факт публикации, asset SHA256 и обратная загрузка](evidence/release-1.0.0-published.json).

UF2 SHA256: `c6cf11961fe335945d69fd30f233090db397745116aa1ecca1661919d2073aca`. PCM2560 (+100% к ранее принятому1280), codec gain60, Apache-2.0 ES8311. Пользователь подтвердил нормальное звучание. Все 10 assets скачаны обратно и побайтно сверены. Build manifest и ZIP сохранены неизменными: их prepublication-статусы — снимок времени сборки; финальное решение отражено отдельными release notes/publication record.

## Проверки и границы

4 native suite: 100640959 core, 2986825 Simon timing, 10746 audio, 2503 codec checks; 18 Python tests; Windows GUI/headless/gallery и ARM Release PASS. Две независимые production-сборки и повторная упаковка идентичны. [CI и waveform comparison](evidence/audio-pcm2560-candidate.json): все CI/local dist-файлы совпали; 6720 PCM отсчётов удвоены с погрешностью округления <=1, форма/длительность сохранены, цифрового клиппинга нет.

На идентифицированной плате установлена 1.0.0/f6c60d7. [Flash evidence](evidence/audio-pcm2560-flash.json): свежий backup8192 вне Git, readback verify, все records/settings до/после равны, электронная самопроверка PASS, BAT_EN HIGH, audio ready/busy=false/faults0. Физическая акустика принята пользователем. Это не новый полный физический прогон всех поздних раундов, LCD/сенсора, батареи и wake: scope каждого прежнего подтверждения сохранён в первичных evidence. [Физический чек-лист](morning-checklist.md) остаётся руководством для повторной проверки, а не заявлением о выполнении каждого пункта на 1.0.

## Лицензии и история

Собственный код MIT, актуальный ES8311 из esp-bsp Apache-2.0 без ограничения на RP2350, все third-party notices включены. [Лицензионный аудит](evidence/licenses-1.0.0.md). Старого restricted драйвера нет в source snapshot/новых ZIP. Доступные Git objects/worktree/прежние assets и новые пакеты проверялись на секреты, findings0; ограниченность детекторов отмечена в [Public Readiness](evidence/public-readiness-1.0.0.md).

LICENSE-002 по прежнему restricted коду в старой публичной истории/assets остаётся открытой задачей. Новый чистый snapshot распространяется отдельно, новый Apache grant не объявляется ретроактивным разрешением. Старые материалы не удалялись/не переписывались. Это отдельный риск прежнего распространения, не скрытый компонент нового бинарника.

PR #1 открыт, merge main не выполнен. Видимость уже была public до аудита и агентом не менялась. [Бэклог](backlog.md), [архив прогресса перед выпуском](evidence/progress-through-1.0.0-prepublication.md), [ранний архив](evidence/progress-through-0.3.5.md).

## Реализация 1.0.1: копирайт выключения

По запросу пользователя добавлено «© 2026, мальчик Коля + OpenAI» на экран «Выключаюсь» (две строки под основным текстом). Добавлен один глиф© из того же Noto Sans Mono; все164 прежних глифа побайтно сохранены. Только render/font, порядок PWR/save/power cut, игры и звукPCM2560 не меняются. После отдельного разрешения установлен1.0.1/0cf9d59; надпись принята пользователем, стабильная1.0.1 опубликована. Опубликованная1.0.0 неизменна.

Проверено программно: 4 native suite и 18 Python tests PASS, галерея симулятора просмотрена — © поддерживается, обе строки помещаются. [Экран](evidence/shutdown-copyright-1.0.1.png), [проверки](evidence/shutdown-copyright-1.0.1.json). Кандидат0cf9d59 собран воспроизводимо, CI PASS; все скачанные dist-файлы побайтно совпали с локальными. [UF2, SHA256 и CI](evidence/shutdown-copyright-1.0.1-candidate.json). Установка1.0.1 выполнена по отдельному разрешению: identity, свежий backup8192 вне Git, readback verify, records/settings comparison и electronic selftest PASS. [Отчёт](evidence/shutdown-copyright-1.0.1-flash.json). BAT_EN HIGH; audio ready=true/busy=false/faults0/PCM2560. Визуальная приёмка копирайта на LCD подтверждена пользователем «надпись ок»; выпуск разрешён «отправь в релиз».
