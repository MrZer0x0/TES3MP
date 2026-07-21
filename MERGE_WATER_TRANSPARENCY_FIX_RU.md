# ArenaMP: объединённый фикс UI + Zero Custom + вода

В пакет уже включены все ранее объединённые исправления, присутствовавшие в полном проекте `ArenaMP_UI_ZeroCustom_settings_full.zip`,
и добавлены новые доработки по воде.

## Новое в этом пакете

- объединён полный актуальный набор фиксов в одном дереве проекта;
- в секцию `[Water]` добавлен параметр `transparency = 1.0`;
- сохранён `refraction scale = 3`;
- в окно настроек воды добавлен новый ползунок `Transparency`;
- восстановлено горячее применение прозрачности через runtime uniform `waterTransparency`;
- исправлена резкая граница волн:
  - средние и мелкие волны теперь плавно затухают по LOD, без жёсткого отключения;
  - у берега сила волн и искажения refraction/refl плавно ослабляются через `shorelineWaveFade`;
  - прозрачность и мутность воды теперь плавно зависят от нового параметра.

## Изменённые файлы

- `apps/openmw/mwrender/water.cpp`
- `files/shaders/water_fragment.glsl`
- `files/mygui/openmw_settings_window.layout`
- `files/settings-default.cfg`

## Установка

### В исходный репозиторий
Распаковать архив changed-files в корень проекта с заменой файлов.

### Полный проект
Использовать полный архив `ArenaMP_all_fixes_water_transparency_smooth_waves_full.zip`.
