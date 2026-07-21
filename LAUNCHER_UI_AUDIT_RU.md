# Проверка Qt UI лаунчера ArenaMP

Проверены все формы, которые перечислены в `apps/launcher/CMakeLists.txt`:

- `advancedpage.ui`
- `contentselector.ui`
- `datafilespage.ui`
- `graphicspage.ui`
- `mainwindow.ui`
- `playpage.ui`
- `settingspage.ui`

## Найденные проблемы

1. `advancedpage.cpp/.hpp` были из Zero Custom, а `advancedpage.ui` — из EncoreMP. В UI отсутствовали 47 объектов OSG/occlusion-настроек, включая `osgTestingGroupBox`, `osgPresetComboBox`, `occlusionCullingCheckBox`, `preloadThreadsSpinBox` и `patchPhysicsThreadsSpinBox`.
2. `mainwindow.ui` компилировался, но оставался вариантом Encore/OpenMW: заголовок `OpenMW Launcher` и фон `openmw-header.png` вместо оформления Zero Custom/TES3MP.

## Выполнено

- восстановлен совместимый `advancedpage.ui` Zero Custom;
- восстановлен `mainwindow.ui` Zero Custom;
- сохранён объединённый `datafilespage.ui` с поддержкой groundcover;
- сохранён ранее восстановленный расширенный `playpage.ui`;
- остальные формы подтверждены как совместимые без изменений.

## Автоматические проверки

- все 7 XML-файлов успешно разобраны;
- исходники и заголовки соответствующих страниц совпадают с Zero Custom;
- набор `objectName` каждой формы совпадает с ожидаемым набором Zero Custom;
- прямые обращения `ui.<member>` не содержат отсутствующих элементов;
- все 7 форм присутствуют в CMake;
- QRC-файлы, указанные в UI, существуют.

Основной `files/settings-default.cfg` не изменялся.
