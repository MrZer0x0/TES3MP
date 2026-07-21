# Исправление сборки ArenaMP

Новая ошибка была вызвана несогласованным переносом occlusion culling:

- `renderingmanager.cpp` использовал `mOcclusionCuller`, `mTerrainOccluder`, `occlusionVisible()` и `rebuildOcclusionBuffer()`;
- соответствующие объявления отсутствовали в `renderingmanager.hpp`;
- `SceneOcclusionCallback` вызывался с четырьмя аргументами, но заголовок содержал старую декларацию;
- `getPagedRefnums()` в реализации ошибочно принимал `std::vector`, тогда как ObjectPaging и Scene используют `std::set`.

Исправлено:

1. Добавлены поля и методы occlusion в `renderingmanager.hpp`.
2. Добавлен согласованный `occlusionculling.hpp`.
3. `getPagedRefnums()` возвращён к `std::set<ESM::RefNum>`.

Скопируйте содержимое архива в корень проекта с заменой файлов.
