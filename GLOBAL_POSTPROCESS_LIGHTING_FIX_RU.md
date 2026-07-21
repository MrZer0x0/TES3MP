# ArenaMP: единый HDR/post-processing и исправление освещения

## Исправлено

- Удалены runtime-множители света из material-шейдеров, которые нарушали расчёт освещения.
- Удалены встроенные геометрические glow/bloom-прибавки из lighting.glsl.
- Восстановлена стандартная модель света OpenMW/Encore с сохранением PBR-блика солнца.
- Удалён отдельный toneMap из water, objects, terrain и groundcover.
- Сцена теперь рендерится один раз в HDR-буфер RGBA16F.
- Один полноэкранный проход обрабатывает всю 3D-картинку: небо, воду, terrain, объекты, траву, частицы и эффекты.
- MyGUI рисуется после post-processing и не меняет цвет.
- HDR, экспозиция, яркость, контраст, насыщенность и bloom применяются горячо ко всей сцене.
- При изменении разрешения или MSAA HDR-буфер переподключается автоматически.
- GUI Scaling Factor перенесён в Detail.
- Отдельная вкладка GUI удалена.
- Небезопасная вкладка Light FX удалена.
- Горячее управление тенями сохранено.

## Значения по умолчанию

- tonemapper = 0 (ACES)
- exposure = 1.0
- gamma = 2.2
- brightness = 1.0
- contrast = 1.0
- saturation = 1.0
- bloom intensity = 0.0

## Изменённые файлы

- apps/openmw/mwrender/renderingmanager.cpp
- apps/openmw/mwrender/renderingmanager.hpp
- files/shaders/postprocess_fragment.glsl
- files/shaders/HDR.glsl
- files/shaders/lighting.glsl
- files/shaders/objects_fragment.glsl
- files/shaders/terrain_fragment.glsl
- files/shaders/groundcover_fragment.glsl
- files/shaders/water_fragment.glsl
- files/shaders/CMakeLists.txt
- files/mygui/openmw_settings_window.layout
- files/settings-default.cfg
- defaults.bin
