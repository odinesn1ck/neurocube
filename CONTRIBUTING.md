# Участие в NeuroCube

Прочитайте [AGENTS](AGENTS.md), [продуктовый замысел](docs/product-vision.md), [архитектуру](docs/architecture.md), [аппаратную конфигурацию](docs/hardware.md) и [политику документации](docs/documentation-policy.md). Текущие задачи — в [backlog](docs/backlog.md), фактические проверки — в [progress](docs/progress.md). Собственный код — [MIT](LICENSE); сохраняйте [сторонние notices](THIRD_PARTY_LICENSES.md).

## Окружение и проверки

Windows, Git и Python 3.12. Пользовательский bootstrap не меняет системный PATH и драйверы:

```powershell
.\tools\bootstrap.ps1
$python = "$env:USERPROFILE\neurocube-tools\venv\Scripts\python.exe"
& $python tools/neurocube.py build --native
& $python -m unittest discover -s tests -p 'test_*.py' -v
& $python docs/tools/check_docs.py
.\build-native\neurocube_sim.exe --headless build-native/menu.ppm
.\build-native\neurocube_sim.exe --gallery artifacts/gallery
.\build-native\neurocube_sim.exe
& $python tools/neurocube.py build
```

[Закреплённые инструменты и восстановление](docs/build.md). Регрессионные suite проверяют игры, генерацию лабиринтов, все 32 раунда Simon, аудиособытия/mute, v1/v2/v3 записи, PWR, idle/wake и QR. Симулятор хранит собственный `sim-records.bin`; запускайте его из изолированного каталога. Windows-аудио — mock. Программные кадры не заменяют LCD или физический ввод.

Для подготовленного релизного коммита с чистым рабочим деревом:

```powershell
& $python tools/prepare_release.py --verify-reproducible
```

Команда создаёт локальные production-сборки, manifest и архивы; она не прошивает плату, не создаёт GitHub release и не публикует проект. [Процесс финализации](docs/evidence/release-1.0.0-readiness.md).

## Изменения кода

C++17, Clean Architecture: игры/domain не импортируют Pico SDK, GPIO или Windows API. Используйте существующие порты, статическую регистрацию и ограниченные буферы. Новую игру в будущем добавляйте независимым `Game` через `GameRegistry`, с переносимыми тестами и метаданными; не привязывайте её к драйверам и не перерабатывайте журнал без отдельного проекта миграции. В 1.0 функциональность заморожена на 0.3.5.

Работайте отдельными коммитами через PR. Финализация проекта ведётся в `codex/neurocube-mvp`; не force push и не merge в main без решения владельца. Опишите фактическое поведение, проверки и ограничения. Обновляйте CHANGELOG/backlog/progress по политике документации; не переносите оперативные дефекты в README. Проверяйте ссылки перед push.

## Аппаратная безопасность

Только RP2350-Touch-LCD-1.54, точная проверка serial и USB topology перед записью. Не угадывайте GPIO. BAT_EN удерживается HIGH; зарядка и защита не перенастраиваются. PWR, PLUS и BOOT имеют разные назначения. Запись только с разрешения владельца и свежим журналом 8192 байта вне Git; после — picotool verify и сравнение records/settings. Образ должен проходить `validate_uf2`: журнал и служебная UF2-страница защищены. Никогда не erase-all, не восстанавливайте заводской дамп поверх пользовательских данных и не меняйте формат записей случайно.

## Публикация и секреты

Не коммитьте токены, `.env`, приватные ключи, журналы пользователя, factory/flash backups и сборочные бинарники. Локальные доказательства хранятся в игнорируемом `artifacts`; в evidence публикуются только проверенные обезличенные итоги. При обнаружении секрета не вставляйте значение в issue/PR/лог: отзовите или ротируйте его через владельца, отдельно согласуйте очистку истории. MIT проекта не даёт права переименовать чужую лицензию.
