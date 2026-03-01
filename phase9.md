# Фаза 9: Потокобезопасность

**Статус:** ✅ Завершена

---

## Постановка задачи

Техническое задание (tz.md, раздел 2.3.4) требует:

> **Потокобезопасность** — базовая синхронизация на уровне менеджера.

До Фазы 9 все публичные методы (`create`, `load`, `destroy`, `allocate`, `deallocate`, `reallocate`) не были защищены от конкурентного доступа из нескольких потоков. При одновременном вызове из разных потоков могло возникнуть неопределённое поведение (гонка данных на внутренних структурах).

---

## Решение

Добавлен статический `std::recursive_mutex s_mutex` в класс `PersistMemoryManager`.

Выбор `std::recursive_mutex` вместо `std::mutex` обоснован тем, что `allocate()` вызывает `expand()`, а `expand()` обновляет `s_instance`, не выходя из `allocate()` — то есть блокировка может быть захвачена повторно одним потоком (например, через `allocate → expand → allocate_from_block`). `std::recursive_mutex` допускает повторный захват тем же потоком.

### Изменения в `include/persist_memory_manager.h`

| Место | Изменение |
|-------|-----------|
| Заголовок файла | Добавлен `#include <mutex>` |
| `private:` | Добавлен `static std::recursive_mutex s_mutex;` |
| Определение статических членов | Добавлен `inline std::recursive_mutex PersistMemoryManager::s_mutex;` |
| `create()` | `std::lock_guard<std::recursive_mutex> lock(s_mutex);` в начале |
| `load()` | `std::lock_guard<std::recursive_mutex> lock(s_mutex);` в начале |
| `destroy()` | `std::lock_guard<std::recursive_mutex> lock(s_mutex);` в начале |
| `allocate()` | `std::lock_guard<std::recursive_mutex> lock(s_mutex);` в начале |
| `deallocate()` | `std::lock_guard<std::recursive_mutex> lock(s_mutex);` в начале |
| `reallocate()` | `std::lock_guard<std::recursive_mutex> lock(s_mutex);` в начале |

---

## Тесты

Созданы 4 теста в `tests/test_thread_safety.cpp`:

| Тест | Описание |
|------|----------|
| `test_concurrent_allocate` | 4 потока параллельно выделяют по 200 блоков по 64 байта. После завершения — `validate()`. |
| `test_concurrent_alloc_dealloc` | 4 потока чередуют allocate/deallocate (500 итераций каждый). После — `validate()` и проверка `allocated_blocks == 0`. |
| `test_concurrent_reallocate` | 4 потока параллельно reallocate своих блоков (100 итераций). После — `validate()`. |
| `test_no_data_races` | 8 потоков выделяют блоки, записывают уникальные значения, проверяют их сохранность. Выявляет гонки данных. |

### Обновлён `tests/CMakeLists.txt`

Тест `test_thread_safety` линкуется с `Threads::Threads` (результат `find_package(Threads REQUIRED)`).

### Обновлён `CMakeLists.txt`

Добавлен `find_package(Threads REQUIRED)`.

---

## Результаты

- Все 4 теста проходят.
- `validate()` подтверждает целостность структур после параллельных операций.
- Версия заголовочного файла обновлена до `0.7.0`.
- Размер заголовочного файла: **1491 строк** (лимит CI: ≤ 1500 строк ✅).

---

## Ограничения

- Мьютекс единый на весь менеджер — блокировка грубая (coarse-grained). При высокой конкуренции это может стать узким местом.
- `save()` и `validate()` не защищены мьютексом (read-only операции; добавление блокировки потребовало бы `shared_mutex` и вышло бы за лимит строк).
- Дальнейшая оптимизация: `std::shared_mutex` для read-only методов — возможная задача для Фазы 10.
