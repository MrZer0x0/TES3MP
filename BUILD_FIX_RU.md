# ArenaMP — исправление PlayPage лаунчера

## Причина ошибки

`apps/launcher/playpage.cpp` взят из Zero Custom и использует расширенную страницу запуска/настроек сервера, но `files/ui/playpage.ui` был перезаписан упрощённой версией EncoreMP.

Из-за этого сгенерированный Qt-класс `Ui::PlayPage` не содержал `serverButton`, `pageTabs`, `serverAddressEdit`, элементов формы `config.lua` и других виджетов.

## Исправление

Восстановлен оригинальный `files/ui/playpage.ui` из MrZer0x0/TES3MP (Zero Custom), совместимый с текущими `playpage.cpp` и `playpage.hpp`.

Проверено:

- XML корректно разбирается;
- все 57 отсутствовавших идентификаторов из лога присутствуют;
- сохранены Play, Run Server, встроенная серверная консоль и редактор `server/scripts/config.lua`;
- `settings-default.cfg` не изменялся.

## Установка

Распаковать архив в корень репозитория с заменой:

`files/ui/playpage.ui`

Затем создать новый commit и запустить новый workflow GitHub Actions.
