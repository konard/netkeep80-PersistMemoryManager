# Comment policy

## Purpose

This document defines the rules for comments in code and canonical documentation.
The goal is to keep the repository free of historical noise while preserving comments
that carry real engineering value.

## Scope

Applies to every file under `include/`, `single_include/`, `tests/`, `demo/`,
`examples/`, `benchmarks/`, `scripts/`, and `docs/`.

## Allowed comment types

| Type                  | Purpose                                              | Example                                                        |
|-----------------------|------------------------------------------------------|----------------------------------------------------------------|
| **Invariant**         | States what must always be true                      | `// left->weight < right->weight after every rebalance`        |
| **Persistence contract** | States what must survive reload / relocation      | `// header CRC is written last so partial writes are detected` |
| **Safety note**       | Warns about UB, corruption, or non-obvious risk      | `// must hold lock — concurrent resize causes UB`              |
| **Design note**       | Short explanation of a non-obvious decision (1-3 lines) | `// AVL instead of RB because height bound matters for mmap` |

If a comment does not fit one of these four types, it should be removed or rewritten
to fit.

## Prohibited comment patterns

The following patterns must not appear in code or canonical docs:

- Issue references: `Issue #N`, `TODO for issue #N`, `implemented in #N`
- Refactoring history: `was previously ...`, `moved from ...`, `renamed from ...`
- Temporal promises: `temporarily left`, `remove later`, `possibly delete`
- Narrative without invariant: multi-line explanations that retell the code
- Obvious restatements: comments that repeat the name of a function or field

History belongs in Git, issues, and pull requests — not in the source tree.

## Comment length rule

If a comment is longer than the code it annotates, it requires explicit justification.
By default, long narratives are removed and the useful part is condensed into
1-3 lines of invariant or design note.

## Documentation rules

Canonical docs (`docs/`) must not contain:

- Chronologies or phase histories
- Lists of closed issues
- "Implemented in version ..." sections
- Issue references as part of normative text

## Related docs

- `tasks/00_REPOSITORY_STYLE.md` — full repository style guide
- `CONTRIBUTING.md` — development workflow and quality standards
