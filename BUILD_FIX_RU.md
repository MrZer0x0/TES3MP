# Полный фикс сборки occlusion culling для ArenaMP

Исправляет несогласованный перенос occlusion culling, проявлявшийся ошибками C2661 в `renderingmanager.cpp`.

Заменяемые файлы:

- `apps/openmw/mwrender/renderingmanager.cpp`
- `apps/openmw/mwrender/renderingmanager.hpp`
- `apps/openmw/mwrender/occlusionculling.hpp`
- `apps/openmw/mwrender/objects.cpp`
- `apps/openmw/mwrender/objects.hpp`
- `apps/openmw/mwrender/objectpaging.cpp`
- `apps/openmw/mwrender/objectpaging.hpp`

Что синхронизировано:

- конструктор `Objects(..., OcclusionCuller*)`;
- конструктор `ObjectPaging(..., OcclusionCuller*)`;
- callback для статических объектов ячейки;
- callback для paged-объектов;
- локальный заголовок `occlusionculling.hpp`;
- поля occlusion в `RenderingManager`;
- единая сигнатура `getPagedRefnums(..., std::set<ESM::RefNum>&)`.

Распакуйте архив в корень проекта с заменой, затем сделайте новый commit и запустите новый workflow GitHub Actions.
