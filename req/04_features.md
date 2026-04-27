# 04. Характеристики продукта (`FEAT`)

Характеристика — одна или несколько логически связанных возможностей системы, представляющих ценность для пользователя и раскрываемых функциональными требованиями.

| ID | Характеристика | Приоритет | Статус | Связанные требования |
|---|---|---|---|---|
| FEAT-001 | Lifecycle management: `create`, `load`, `destroy`, `is_initialized`. | Must | Recovered | FR-001..FR-003 |
| FEAT-002 | Persistent allocator: best-fit allocator поверх intrusive AVL free-tree. | Must | Recovered | FR-004, FR-013, QA-PERF-001 |
| FEAT-003 | Persistent pointers: `pptr<T>` как гранульный индекс вместо raw pointer. | Must | Recovered | FR-007, DR-007, QA-PORT-001 |
| FEAT-004 | Verification and recovery: `verify`, `load(VerifyResult&)`, rebuild/repair служебных структур. | Must | Recovered | FR-014, QA-REL-002, QA-REC-001 |
| FEAT-005 | Root/domain registry для нескольких persistent forest roots. | Should | Recovered | FR-011, FR-012 |
| FEAT-006 | Storage backends: heap, static, mmap. | Should | Recovered | IF-005, SYS-003 |
| FEAT-007 | Lock policies: no-lock и shared-mutex режимы. | Should | Recovered | QA-THREAD-001, QA-THREAD-002 |
| FEAT-008 | Persistent containers/types: `pstringview`, `pstring`, `pmap`, `parray`, `pallocator`. | Should | Recovered | UR-008, DR-009 |
| FEAT-009 | Single-header distribution surface. | Could | Recovered | IF-002, CON-010 |
| FEAT-010 | Diagnostics taxonomy through `VerifyResult`, `PmmError`, violation types. | Should | Recovered | FR-015, QA-DIAG-001 |
