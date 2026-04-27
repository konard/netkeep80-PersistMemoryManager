# 13. Матрица трассируемости

Упрощенная трассировка `BR → UR/FEAT → FR/QA/DR/IF/CON → AC`.

| Бизнес-цель | Пользовательские/feature требования | Реализационные требования | Критерии приемки |
|---|---|---|---|
| BR-001 | UR-001, FEAT-001 | FR-001, FR-002, FR-003, CON-003, CON-004 | AC-001, AC-006 |
| BR-002 | UR-003, FEAT-003 | FR-007, DR-007, QA-PORT-001, SYS-003 | AC-004 |
| BR-003 | UR-004, UR-005, FEAT-004 | FR-014, QA-REL-001, QA-REL-002, QA-REC-001, QA-DIAG-001 | AC-005, AC-006, AC-010 |
| BR-004 | FEAT-004, FEAT-010 | RULE-003, RULE-004, CON-006, QA-MAINT-001 | AC-011 |
| BR-005 | UR-007, UR-008, FEAT-005, FEAT-008 | FR-011, FR-012, DR-009, SYS-004 | AC-002, AC-003 |
| BR-006 | FEAT-009 | RULE-005, CON-010, QA-MAINT-001 | AC-011 |

## Feature-level trace

| Feature | Functional/data/interface requirements | Quality/constraints |
|---|---|---|
| FEAT-001 | FR-001, FR-002, FR-003 | CON-004 |
| FEAT-002 | FR-004, FR-013, DR-005, DR-006 | QA-PERF-001 |
| FEAT-003 | FR-007, FR-008, DR-007 | QA-PORT-001, CON-007 |
| FEAT-004 | FR-014, FR-015 | QA-REL-001, QA-REL-002, QA-REC-001, QA-DIAG-001 |
| FEAT-005 | FR-011, FR-012, DR-009 | SYS-004 |
| FEAT-006 | IF-005, SYS-003 | CON-008 |
| FEAT-007 | IF-006 | QA-THREAD-001, QA-THREAD-002, CON-009 |
| FEAT-008 | FR-017, FR-018, DR-009, DR-010, DR-011 | CON-012 |
| FEAT-009 | IF-002 | RULE-005, CON-010 |
| FEAT-010 | FR-015, IF-010 | QA-DIAG-001 |
