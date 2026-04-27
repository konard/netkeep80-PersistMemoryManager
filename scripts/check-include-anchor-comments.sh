#!/usr/bin/env bash
set -euo pipefail

python3 - "$@" <<'PY'
import pathlib
import re
import sys

ANCHOR_RE = re.compile(r"^/\*\n(#+) ([^\n]+)\n\*/$")
INDENTED_ANCHOR_RE = re.compile(r"^/\*\n\s+#+ [^\n]+\n\s*\*/$")
INDENTED_CLOSE_RE = re.compile(r"^/\*\n#+ [^\n]+\n\s+\*/$")
ANCHOR_NAME_RE = re.compile(r"^[a-z_~][a-z0-9_~]*(?:-[a-z_~][a-z0-9_~]*)*$")


def line_number_at(text, pos):
    return text.count("\n", 0, pos) + 1


def anchor_segment_count(anchor):
    return anchor.count("-") + 1


def validate_anchor_comment(comment):
    match = ANCHOR_RE.fullmatch(comment)
    if not match:
        if INDENTED_ANCHOR_RE.fullmatch(comment):
            return "anchor heading marker must start at the beginning of the line"
        if INDENTED_CLOSE_RE.fullmatch(comment):
            return "anchor closing marker must start at the beginning of the line"
        return "block comment is not a PMM anchor"

    heading_depth = len(match.group(1))
    anchor = match.group(2)
    if not ANCHOR_NAME_RE.fullmatch(anchor):
        return "anchor name must contain only lowercase ASCII hyphen-separated segments"
    if heading_depth != anchor_segment_count(anchor):
        return "anchor heading depth must match the number of hyphen-separated name segments"
    return ""


def skip_quoted_literal(text, pos):
    quote = text[pos]
    pos += 1
    while pos < len(text):
        if text[pos] == "\\":
            pos += 2
            continue
        if text[pos] == quote:
            return pos + 1
        pos += 1
    return pos


def skip_raw_string_literal(text, pos):
    open_pos = text.find("(", pos + 2)
    if open_pos == -1:
        return pos + 2
    delimiter = text[pos + 2 : open_pos]
    close_token = ")" + delimiter + '"'
    close_pos = text.find(close_token, open_pos + 1)
    if close_pos == -1:
        return len(text)
    return close_pos + len(close_token)


def validate_file(path):
    text = path.read_text(encoding="utf-8")
    failures = []
    pos = 0
    while pos < len(text):
        char = text[pos]
        if char in {'"', "'"}:
            pos = skip_quoted_literal(text, pos)
            continue
        if char == "R" and pos + 1 < len(text) and text[pos + 1] == '"':
            pos = skip_raw_string_literal(text, pos)
            continue

        if pos + 1 < len(text) and text[pos : pos + 2] == "//":
            failures.append(f"{path}:{line_number_at(text, pos)}: line comment is not an anchor")
            pos += 2
            continue

        if pos + 1 < len(text) and text[pos : pos + 2] == "/*":
            if pos > 0 and text[pos - 1] != "\n":
                failures.append(
                    f"{path}:{line_number_at(text, pos)}: "
                    "anchor block comment must start at the beginning of the line"
                )

            end = text.find("*/", pos + 2)
            if end == -1:
                failures.append(f"{path}:{line_number_at(text, pos)}: unterminated block comment")
                break

            comment = text[pos : end + 2]
            validation_failure = validate_anchor_comment(comment)
            if validation_failure:
                failures.append(f"{path}:{line_number_at(text, pos)}: {validation_failure}")
            pos = end + 2
            continue

        pos += 1
    return failures


def default_files():
    return sorted(
        path
        for path in pathlib.Path("include/pmm").rglob("*")
        if path.is_file() and path.suffix in {".h", ".inc"}
    )


files = [pathlib.Path(arg) for arg in sys.argv[1:]] if len(sys.argv) > 1 else default_files()
failures = []
for path in files:
    if path.is_file() and path.suffix in {".h", ".inc"}:
        failures.extend(validate_file(path))

if failures:
    print("Include anchor comment check failed:")
    for failure in failures:
        print(f"  - {failure}")
    sys.exit(1)

print("OK: include anchor comments use beginning-of-line PMM anchors")
PY
