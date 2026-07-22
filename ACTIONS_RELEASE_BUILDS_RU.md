# GitHub Actions: Release-сборки ArenaMP

Добавлен workflow `.github/workflows/release-builds.yml`.

## Запуск

1. Откройте репозиторий на GitHub.
2. Перейдите в **Actions → Release builds → Run workflow**.
3. Выберите платформы и запустите workflow.
4. После завершения откройте страницу запуска. Архивы находятся в секции **Artifacts**.

Workflow также запускается автоматически при отправке тега вида `v*`.

## Артефакты

- `ArenaMP-Windows-x64-<tag>.zip`
- `ArenaMP-Linux-x86_64-SteamDeck-<tag>.tar.gz`
- `ArenaMP-macOS-arm64-<tag>-experimental`

Срок хранения на странице Actions: 30 дней. Он может быть ограничен настройками репозитория или организации.

## Состояние платформ

### Windows x64

Основная сборка. Используется существующий скрипт `CI/before_script.msvc.sh`, затем `cmake --build` и `cmake --install`.

### Linux x86_64 / Steam Deck

Steam Deck использует x86_64 Linux, поэтому отдельная архитектура не требуется. Архив собирается на Ubuntu 22.04 и предназначен для запуска в Desktop Mode/SteamOS. Для максимальной переносимости в будущем желательно собирать AppImage или использовать Steam Runtime SDK.

### macOS ARM64

Экспериментальная сборка на нативном runner `macos-14` (Apple Silicon). Она отключена при ручном запуске по умолчанию и имеет `continue-on-error`, потому что старый код OpenMW/TES3MP 0.47 и его CMake-модули могут быть несовместимы с современными ARM64-версиями Qt 5, MyGUI, OpenSceneGraph и FFmpeg.

Старый `CI/before_install.osx.sh` не используется: он скачивает готовый dependency bundle 2021 года, который нельзя считать ARM64-совместимым. Вместо него workflow ставит нативные зависимости через Homebrew.

## Важное ограничение

Workflow подготовлен и синтаксически проверен, но реальные бинарники можно подтвердить только запуском в GitHub Actions. Если macOS job упадёт, журнал покажет конкретную несовместимую зависимость или CMake-модуль.
