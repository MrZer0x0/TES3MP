# ArenaMP

ArenaMP — форк TES3MP 0.8.1 на базе OpenMW 0.47.0. Ветка содержит клиент, выделенный сервер, браузер серверов, лаунчер, обновлённые HUD/GUI-настройки, графические исправления и интеграцию контента EncoreMP 0.92.

> Для запуска нужна легальная копия **The Elder Scrolls III: Morrowind**. Игровые файлы Bethesda в репозиторий не входят.

## Возможности

- мультиплеерный клиент и сервер TES3MP;
- лаунчер и браузер серверов;
- расширенные настройки интерфейса, HUD, Quick Loot и камеры;
- правки воды, каустики, освещения, теней и постобработки;
- сохранение совместимости с сетевым протоколом TES3MP 0.8.1;
- включённые ESP-файлы EncoreMP 0.92 для серверов, которые используют этот набор правил.

## Состав после сборки

| Файл | Назначение |
|---|---|
| `tes3mp` | игровой клиент ArenaMP/TES3MP |
| `tes3mp-server` | выделенный сервер |
| `tes3mp-browser` | браузер серверов |
| `openmw-launcher` | настройка игры, модов и путей к данным |
| `openmw-wizard` | мастер импорта настроек, если включён при сборке |

## ESP-файлы EncoreMP

В корне лежат готовые ESP-файлы:

- `EncoreMPV092.ESP` — основной файл EncoreMP;
- `EncoreMPV092newcontent.ESP` — дополнительный контент;
- `EncoreMPV092Spells1Base.ESP` — заклинания для базовой игры;
- `EncoreMPV092Spells2TRcore.ESP` — базовая игра и Tamriel Rebuilt core;
- `EncoreMPV092Spells3TRall.ESP` — расширенный набор Tamriel Rebuilt.

Используйте только **один** файл `EncoreMPV092Spells*` одновременно. Порядок ESP на клиенте должен совпадать с порядком на сервере.

## Получение исходников

```bash
git clone --recurse-submodules https://github.com/MrZer0x0/TES3MP.git
cd TES3MP
```

Для уже скачанного репозитория:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

CrabNet/RakNet должен быть в `extern/raknet`. В актуальном CI добавлена проверка `CI/ensure-bundled-deps.sh`: если каталог отсутствует или неполный, скрипт пытается загрузить `TES3MP/CrabNet` перед конфигурацией CMake.

## Быстрая сборка Linux x86_64

Проверенная CI-конфигурация использует Ubuntu 22.04 и Ninja.

```bash
CI/ensure-bundled-deps.sh
bash CI/linux/install-deps.sh
bash CI/linux/configure.sh
bash CI/linux/build-package.sh
```

Архив появится в `artifacts/`:

```text
ArenaMP-Linux-x86_64-<ветка-или-тег>.tar.gz
```

## Быстрая сборка macOS arm64

Проверенная CI-конфигурация использует Apple Silicon runner, Homebrew и минимальную цель macOS 12.

```bash
CI/ensure-bundled-deps.sh
bash CI/macos-arm64/install-deps.sh
bash CI/macos-arm64/configure.sh
bash CI/macos-arm64/build-package.sh
```

macOS-сборка использует встроенный TinyXML из `extern/oics`, поэтому внешний репозиторий TinyXML больше не клонируется. MyGUI собирается в `${RUNNER_TEMP}/MyGUI/install` и кэшируется в GitHub Actions.

Готовый `.dmg` или резервный `.tar.gz` появится в `artifacts/`.

## Windows

Для Windows оставлены существующие сценарии MSVC и `appveyor.yml`. Рекомендуемая среда: Visual Studio 2022 x64, Windows SDK, CMake и Ninja. Windows-сборка этим архивом не отключалась.

## GitHub Actions

Workflow находится в `.github/workflows/release-builds.yml`. Старый падающий workflow должен быть заменён именно этим файлом, иначе GitHub продолжит запускать прежние шаги.

Он запускает две независимые задачи:

- `Linux x86_64` на `ubuntu-22.04`;
- `macOS arm64` на `macos-26-arm64`.

Запуск выполняется при push в `Main`, pull request, теге `v*` или вручную через `workflow_dispatch`. Готовые пакеты публикуются как GitHub Actions artifacts.

## Основные CMake-параметры

| Параметр | Linux CI | macOS CI | Назначение |
|---|---:|---:|---|
| `BUILD_OPENCS` | `OFF` | `OFF` | не собирать OpenMW-CS в CI-пакетах |
| `BUILD_WIZARD` | `OFF` | `OFF` | не собирать мастер импорта в CI-пакетах |
| `BUILD_UNITTESTS` | `OFF` | `OFF` | не собирать тесты в релизном пакете |
| `USE_SYSTEM_TINYXML` | `ON` | `OFF` | Linux берёт пакет, macOS берёт встроенный TinyXML |
| `OPENMW_USE_SYSTEM_MYGUI` | `ON` | `ON` | использовать установленный/собранный MyGUI |
| `OPENMW_USE_SYSTEM_OSG` | `ON` | `ON` | использовать системный OpenSceneGraph |
| `OPENMW_USE_SYSTEM_BULLET` | `ON` | `ON` | использовать системный Bullet |

## Настройка клиента

1. Запустите `openmw-launcher`.
2. Добавьте путь к установленному Morrowind.
3. Включите `Morrowind.esm`, `Tribunal.esm` и `Bloodmoon.esm` в правильном порядке.
4. Добавьте ESP-файлы, которые использует сервер.
5. Проверьте, что порядок модов на клиенте совпадает с сервером.

Главные файлы настроек:

```text
files/settings-default.cfg
files/tes3mp/tes3mp-client-default.cfg
files/tes3mp/tes3mp-server-default.cfg
```

## Частые ошибки сборки

### `The submodules were not downloaded`

Обычно это означает, что `extern/raknet` отсутствует или содержит только пустой каталог. Запустите:

```bash
CI/ensure-bundled-deps.sh
```

Затем удалите `build/` и повторите конфигурацию.

### `Remote branch 2.6.2 not found`

Это ошибка старого macOS-скрипта, который пытался клонировать несуществующую ветку TinyXML. В текущем `CI/macos-arm64/install-deps.sh` этот шаг удалён: используется встроенный TinyXML.

### macOS не находит OpenAL, Qt или LuaJIT

Удалите `build/`, проверьте Homebrew-пакеты и повторите:

```bash
CI/macos-arm64/install-deps.sh
CI/macos-arm64/configure.sh
```

### CPack не создал `.dmg`

`CI/macos-arm64/build-package.sh` теперь не роняет весь job только из-за DMG. Если `DragNDrop` не сработал, создаётся fallback-архив `.tar.gz` из `stage/`.

## Структура репозитория

```text
apps/        приложения и точки входа
components/  общие подсистемы движка
files/       ресурсы и конфигурации по умолчанию
extern/      встроенные сторонние библиотеки
CI/          скрипты установки, конфигурации и упаковки
cmake/       CMake-модули
.github/     GitHub Actions workflow
```

## Что прикладывать к issue

- ОС и архитектуру;
- полный лог сборки;
- имя workflow/job или локальную команду;
- ветку, тег или commit hash;
- `build/CMakeCache.txt`, если он был создан.

## Лицензия

Лицензия находится в `LICENSE`. Авторы указаны в `AUTHORS.md`, история изменений — в `CHANGELOG.md`.
