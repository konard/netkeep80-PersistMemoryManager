# 02. Бизнес-правила (`RULE`)

Бизнес-правило — политика, предписание, стандарт или правило, которое определяет или ограничивает бизнес-процесс/разработку и может порождать требования.

| ID | Правило | Приоритет | Статус | Основание |
|---|---|---|---|---|
| RULE-001 | Все персистентные ссылки внутри PAP должны храниться как offset/granule indices, а не как raw pointers. | Must | Recovered | README, `docs/architecture.md` |
| RULE-002 | PMM не должен интерпретировать schema/payload верхнего уровня. | Must | Recovered | `docs/pmm_target_model.md` |
| RULE-003 | Любое изменение PMM должно быть трассируемо к разрешенным направлениям развития: hardening, compaction, extraction prep, intrusive-index formalization, validation/invariants/recovery strengthening. | Must | Recovered | `docs/pmm_target_model.md` |
| RULE-004 | PMM не должен включать query engine, VM/execution engine, replication/sync, business logic или product/application semantics. | Must | Recovered | `docs/pmm_target_model.md` |
| RULE-005 | Generated surface, например `single_include/`, не должен смешиваться с core changes в одном изменении. | Should | Recovered | `docs/pmm_target_model.md`, README |
| RULE-006 | `verify()` должен оставаться диагностической операцией без модификации образа; восстановление допускается в документированных фазах `load`/repair. | Must | Recovered | README, `docs/validation_model.md` |
| RULE-007 | `create_typed` должен использоваться только для типов с nothrow-конструктором; `destroy_typed` — только для типов с nothrow-деструктором. | Must | Recovered | README API section |
| RULE-008 | Persistent container/domain semantics должны храниться в forest/domain registry, не в разрозненных глобальных runtime-таблицах. | Should | Recovered | README, `docs/architecture.md` |
| RULE-009 | `NoLock` допустим только для single-threaded сценариев; многопоточный доступ должен использовать `SharedMutexLock` или эквивалентную lock policy. | Must | Recovered | README, `docs/thread_safety.md` |
