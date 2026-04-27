# 03. Пользовательские требования (`UR`)

Пользовательское требование описывает задачу, которую определенный класс пользователей должен иметь возможность выполнять, или требуемый атрибут продукта.

| ID | Пользовательское требование | Приоритет | Статус | Основание |
|---|---|---|---|---|
| UR-001 | Как интегратор библиотеки, я хочу создать, загрузить и уничтожить PAP через `create`, `load`, `destroy`, чтобы управлять жизненным циклом персистентной области. | Must | Recovered | README |
| UR-002 | Как разработчик persistent-структур, я хочу выделять и освобождать память через typed и untyped allocation API. | Must | Recovered | README |
| UR-003 | Как разработчик, я хочу хранить `pptr<T>` в persistent-структурах и разрешать его после повторной загрузки образа. | Must | Recovered | README, `docs/architecture.md` |
| UR-004 | Как разработчик, я хочу выполнить `verify()` без модификации образа. | Must | Recovered | README, `docs/validation_model.md` |
| UR-005 | Как разработчик, я хочу загружать образ через `load(VerifyResult&)`, получая диагностику и документированное восстановление служебных структур. | Must | Recovered | README |
| UR-006 | Как интегратор, я хочу выбрать готовый preset: embedded/static, single-threaded, multi-threaded, industrial DB, large DB. | Should | Recovered | README preset table |
| UR-007 | Как разработчик контейнеров, я хочу использовать legacy root pointer и named forest/domain registry для нескольких persistent roots. | Should | Recovered | README |
| UR-008 | Как пользователь библиотеки, я хочу иметь базовые persistent-типы: `pstring`, `pstringview`, `pmap`, `parray`, `pallocator`. | Should | Recovered | README |
| UR-009 | Как сопровождающий, я хочу запускать CMake/CTest и regression tests, чтобы проверять корректность изменений. | Should | Recovered | README |
| UR-010 | Как разработчик embedded-сценариев, я хочу использовать статический storage без heap allocation. | Could | Recovered | README preset table |
| UR-011 | Как разработчик file-backed persistence, я хочу использовать `MMapStorage` и helper-функции сохранения/загрузки. | Could | Recovered | README, `docs/architecture.md` |
