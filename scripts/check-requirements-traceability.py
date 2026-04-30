#!/usr/bin/env python3
"""Validate traceability of the PMM requirements catalog.

Invariants enforced:

1. Every ``##``-level heading in ``req/*.md`` is a requirement identifier in
   format ``ttt-xxx`` or ``ttt-<sub>-xxx`` (lowercase ASCII, hyphen-separated).
2. The numeric file-name prefix matches the requirement type (see TYPE_FILE).
3. Each requirement has at least one downstream link: a child requirement, a
   source file reference, or a test file reference (``Реализуется в:``,
   ``Проверяется в:``).
4. Every internal markdown anchor link of the form ``foo.md#anchor`` resolves
   to an actual ``##``-level anchor in the target file.
5. Every ``req:`` annotation in ``include/pmm/**.h`` and ``tests/**.cpp``
   references a known requirement ID. (PMM anchor format
   ``/*\n## anchor\nreq: id1, id2\n*/``.)

Outputs human-readable error report; exits non-zero on any violation.
"""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
REQ_DIR = ROOT / "req"
INC_DIR = ROOT / "include" / "pmm"
TESTS_DIR = ROOT / "tests"

# Type prefix -> req file. Files not in this map are exempted from rule 2 (e.g.
# README.md, traceability matrix).
TYPE_FILE = {
    "br": "01_business_requirements.md",
    "rule": "02_business_rules.md",
    "ur": "03_user_requirements.md",
    "feat": "04_features.md",
    "fr": "05_functional_requirements.md",
    "dr": "06_data_requirements.md",
    "if": "07_external_interfaces.md",
    "qa": "08_quality_attributes.md",
    "con": "09_constraints.md",
    "sys": "10_system_requirements.md",
    "asm": "11_assumptions_dependencies.md",
    "dep": "11_assumptions_dependencies.md",
    "ac": "12_acceptance_criteria.md",
}

REQ_ID_RE = re.compile(r"^[a-z]+(?:-[a-z0-9]+)+$")
H2_RE = re.compile(r"^## ([^\n]+)$", re.MULTILINE)
LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")
ANCHOR_RE = re.compile(
    r"^/\*\n(#+) ([^\n]+)\n((?:req:[^\n]+\n)*)\*/$",
    re.MULTILINE,
)
REQ_LINE_RE = re.compile(r"^req:\s*([a-z][a-z0-9-]*(?:\s*,\s*[a-z][a-z0-9-]*)*)\s*$")


def collect_md_anchors() -> tuple[dict[str, set[str]], list[str]]:
    """Return ({file -> set(anchor)}, errors)."""
    anchors: dict[str, set[str]] = {}
    errors: list[str] = []
    for md in sorted(REQ_DIR.glob("*.md")):
        text = md.read_text(encoding="utf-8")
        ids: set[str] = set()
        for m in H2_RE.finditer(text):
            heading = m.group(1).strip()
            ids.add(heading)
        anchors[md.name] = ids
    return anchors, errors


def expected_file_for(req_id: str) -> str | None:
    # Try longest prefix first (e.g. "asm" before "as").
    parts = req_id.split("-")
    if not parts:
        return None
    prefix = parts[0]
    return TYPE_FILE.get(prefix)


def section_blocks(text: str) -> list[tuple[str, str]]:
    """Yield (heading, body) for ## sections in the given markdown text."""
    matches = list(H2_RE.finditer(text))
    blocks: list[tuple[str, str]] = []
    for i, m in enumerate(matches):
        heading = m.group(1).strip()
        start = m.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
        blocks.append((heading, text[start:end]))
    return blocks


def check_requirements(anchors: dict[str, set[str]]) -> list[str]:
    errors: list[str] = []

    # Collect every known req ID across all files.
    known_ids: set[str] = set()
    for md, ids in anchors.items():
        if md in {"README.md", "13_traceability_matrix.md"}:
            continue
        known_ids.update(ids)

    # Enforce ID format and file-name match; require at least one downstream
    # reference per requirement.
    for md_path in sorted(REQ_DIR.glob("*.md")):
        if md_path.name in {"README.md", "13_traceability_matrix.md"}:
            continue
        text = md_path.read_text(encoding="utf-8")
        for heading, body in section_blocks(text):
            if not REQ_ID_RE.match(heading):
                errors.append(
                    f"{md_path.name}: heading '## {heading}' is not a valid "
                    f"requirement id (expected lowercase ttt-xxx)"
                )
                continue
            expected_file = expected_file_for(heading)
            if expected_file and expected_file != md_path.name:
                errors.append(
                    f"{md_path.name}: id '{heading}' belongs in "
                    f"{expected_file}"
                )

            # A downstream reference can appear under several markers:
            #  - 'Реализуется в:' (implementation file/sub-requirement),
            #  - 'Проверяется в:' (acceptance criteria/test),
            #  - 'Тесты:' (test files, used in 12_acceptance_criteria.md),
            #  - 'Проверяет:' (links from ac-XXX up to the requirements they
            #    cover — counted as traceability),
            #  - 'Связано с:' (used in 11_assumptions_dependencies.md and ac).
            has_downstream = False
            for marker_re in (
                r"\*\*Реализуется в:\*\*([^\n]*)\n((?:  -[^\n]+\n)*)",
                r"\*\*Проверяется в:\*\*([^\n]+)",
                r"\*\*Тесты:\*\*([^\n]+)",
                r"\*\*Проверяет:\*\*([^\n]+)",
                r"\*\*Связано с:\*\*([^\n]+)",
            ):
                m2 = re.search(marker_re, body)
                if m2 and any(g for g in m2.groups() if g and g.strip()):
                    has_downstream = True
                    break
            if not has_downstream:
                errors.append(
                    f"{md_path.name}#{heading}: missing downstream reference "
                    f"(expected one of: 'Реализуется в:', 'Проверяется в:', "
                    f"'Тесты:', 'Проверяет:', 'Связано с:')"
                )

            # Validate every cross-file link inside this section block.
            for link_text, target in LINK_RE.findall(body):
                if "#" not in target:
                    continue
                file_part, _, anchor_part = target.partition("#")
                if file_part.startswith("http") or not file_part.endswith(".md"):
                    continue
                if file_part not in anchors:
                    errors.append(
                        f"{md_path.name}#{heading}: link to unknown file "
                        f"'{file_part}'"
                    )
                    continue
                if anchor_part and anchor_part not in anchors[file_part]:
                    errors.append(
                        f"{md_path.name}#{heading}: anchor "
                        f"'{file_part}#{anchor_part}' not found"
                    )
    return errors


def check_source_anchors(known_ids: set[str]) -> list[str]:
    errors: list[str] = []
    for path in sorted(INC_DIR.rglob("*.h")):
        text = path.read_text(encoding="utf-8")
        for m in ANCHOR_RE.finditer(text):
            depth = m.group(1)
            anchor_name = m.group(2).strip()
            req_block = m.group(3)
            if not req_block.strip():
                continue
            for line in req_block.strip().splitlines():
                rm = REQ_LINE_RE.match(line.strip())
                if not rm:
                    errors.append(
                        f"{path.relative_to(ROOT)}: anchor '{anchor_name}' "
                        f"has malformed req line '{line.strip()}'"
                    )
                    continue
                ids = [x.strip() for x in rm.group(1).split(",") if x.strip()]
                for rid in ids:
                    if rid not in known_ids:
                        errors.append(
                            f"{path.relative_to(ROOT)}: anchor '{anchor_name}' "
                            f"references unknown requirement '{rid}'"
                        )
    return errors


def check_test_anchors(known_ids: set[str]) -> list[str]:
    errors: list[str] = []
    if not TESTS_DIR.exists():
        return errors
    for path in sorted(TESTS_DIR.rglob("*.cpp")):
        text = path.read_text(encoding="utf-8")
        for m in ANCHOR_RE.finditer(text):
            anchor_name = m.group(2).strip()
            req_block = m.group(3)
            if not req_block.strip():
                continue
            for line in req_block.strip().splitlines():
                rm = REQ_LINE_RE.match(line.strip())
                if not rm:
                    errors.append(
                        f"{path.relative_to(ROOT)}: anchor '{anchor_name}' "
                        f"has malformed req line '{line.strip()}'"
                    )
                    continue
                ids = [x.strip() for x in rm.group(1).split(",") if x.strip()]
                for rid in ids:
                    if rid not in known_ids:
                        errors.append(
                            f"{path.relative_to(ROOT)}: anchor '{anchor_name}' "
                            f"references unknown requirement '{rid}'"
                        )
    return errors


def main() -> int:
    anchors, errors = collect_md_anchors()
    errors.extend(check_requirements(anchors))

    known_ids: set[str] = set()
    for md, ids in anchors.items():
        if md in {"README.md", "13_traceability_matrix.md"}:
            continue
        known_ids.update(ids)

    errors.extend(check_source_anchors(known_ids))
    errors.extend(check_test_anchors(known_ids))

    if errors:
        for e in errors:
            print(e, file=sys.stderr)
        print(f"\nFAIL: {len(errors)} traceability violation(s)", file=sys.stderr)
        return 1
    print("OK: requirements traceability is consistent")
    return 0


if __name__ == "__main__":
    sys.exit(main())
