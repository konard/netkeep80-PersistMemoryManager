#!/usr/bin/env bash
set -euo pipefail

python3 - "$@" <<'PY'
import pathlib
import re
import sys

ANCHOR_RE = re.compile(r"^/\*\n(#+) ([^\n]+)\n((?:req:[^\n]+\n)*)\*/$")
INDENTED_ANCHOR_RE = re.compile(r"^/\*\n\s+#+ [^\n]+\n\s*\*/$")
INDENTED_CLOSE_RE = re.compile(r"^/\*\n#+ [^\n]+\n\s+\*/$")
ANCHOR_NAME_RE = re.compile(r"^[a-z_~][a-z0-9_~]*(?:-[a-z_~][a-z0-9_~]*)*$")
# Optional req-trace lines inside an anchor comment. Format:
#   req: <id>[, <id>...]
# where <id> is a lowercase requirement anchor like fr-021, qa-rel-001, ac-003.
REQ_LINE_RE = re.compile(r"^req:\s*([a-z][a-z0-9-]*(?:\s*,\s*[a-z][a-z0-9-]*)*)\s*$")
REQ_ID_RE = re.compile(r"^[a-z]+(?:-[a-z0-9]+)+$")


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

    req_block = match.group(3)
    if req_block:
        for line in req_block.rstrip("\n").split("\n"):
            req_match = REQ_LINE_RE.fullmatch(line)
            if not req_match:
                return f"invalid req-trace line in anchor: {line!r}"
            for req_id in (s.strip() for s in req_match.group(1).split(",")):
                if not REQ_ID_RE.fullmatch(req_id):
                    return f"invalid requirement id in anchor: {req_id!r}"
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
