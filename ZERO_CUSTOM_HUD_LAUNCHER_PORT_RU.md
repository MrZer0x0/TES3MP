# Порт HUD и лаунчера Zero Custom в ArenaMP

Источник: MrZer0x0/TES3MP, ветка Main, архив TES3MP-Main(1).zip.

Перенесено:
- полный каталог apps/launcher Zero Custom;
- serverdialog и зависимость Qt5::Network;
- Zero Custom HUD C++ (hud.cpp/hud.hpp);
- HUD layouts, skins, layers и progress skin;
- интерфейс чата;
- шрифты Ayembedt и Pelagiad;
- текстуры crosshair.dds и sneak.dds.

Сохранено из ArenaMP:
- openmw_settings_window.layout;
- settingswindow.cpp/.hpp;
- горячее изменение GUI scaling factor;
- расширенные настройки воды/PBR;
- вода, PBR, occlusion и изменения EncoreMP.

Примечание: глобальный openmw_windows.skin.xml взят из Zero Custom, поскольку стили HUD зависят от него.
