# Comment and documentation policy

This is the canonical policy for PMM text discipline: code comments,
documentation placement, and review criteria for text changes. `CONTRIBUTING.md`
links here instead of restating these rules.

## Canonical places

One concept has one canonical place:

- PMM role and boundaries live in [pmm_target_model.md](pmm_target_model.md).
- Operational transformation rules live in [pmm_transformation_rules.md](pmm_transformation_rules.md).
- Text discipline lives in this document.
- Contributor workflow lives in [../CONTRIBUTING.md](../CONTRIBUTING.md).

Duplicate explanations must be removed or replaced with links to the canonical
document.

## Comments

Comments are allowed only when they carry knowledge not derivable from the code:

- **Invariant**: what must always remain true.
- **Persistence contract**: what must survive reload, relocation, or crash.
- **Safety note**: non-obvious UB, corruption, ordering, or concurrency risk.
- **Short design note**: a compact reason for a non-obvious decision.

Prohibited comment patterns:

- historical or issue narrative: `Issue #N`, `implemented in #N`, `moved from`;
- tutorial-style prose in the kernel;
- restating a function, field, or type name;
- temporary promises: `remove later`, `temporary`, `possibly delete`;
- long narrative without an invariant, contract, safety note, or design reason.

History belongs in Git, issues, and pull requests, not in the source tree.

`include/pmm` block comments are reserved for code anchors. The anchor line must
be lowercase, use `-` between name segments, and contain as many name segments as
heading markers: `## pmm-type`, `### pmm-type-member`.

## Documentation

Canonical docs are limited to these classes:

- target model;
- transformation rules;
- contracts and invariants;
- public API or usage index;
- comment and docs policy.

Other text is collapsed into an existing canonical document, archived when it has
historical value, or left in issue / PR discussion. A new markdown file is an
exception, not the norm.

## Surface rule

PMM text surface should shrink over time. New text must replace older text or
explicitly justify temporary surface debt in the issue and PR. Duplicating text
beside the new version is not allowed.

Docs and comments PRs are reviewed first for:

- no duplicated policy or architecture text;
- non-positive text surface delta by default;
- correct canonical placement;
- no process-noise, phase logs, roadmap dumps, or temporary notes.
