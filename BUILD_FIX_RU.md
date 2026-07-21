# Исправление сборки лаунчера ArenaMP

Исправлена неполная интеграция groundcover в Zero Custom launcher.

Заменяемые файлы:
- `components/config/launchersettings.hpp`
- `components/config/launchersettings.cpp`
- `components/config/gamesettings.hpp`
- `components/config/gamesettings.cpp`
- `files/ui/datafilespage.ui`

Добавлено:
- checkbox `groundcoverCheckBox`;
- хранение списка groundcover отдельно от обычного content;
- `setGroundcoverList` / `getGroundcoverList`;
- `getGroundcoverFiles` / `isGroundcoverEnabled`;
- четырёхпараметрический `setContentList`;
- корректное копирование и удаление профилей groundcover.

Основной `settings-default.cfg` не изменяется.
