#!/usr/bin/env bash
set -euo pipefail

FAILED=0

# --- 1. Extract version from each canonical source ---

# CMakeLists.txt: project(PersistMemoryManager VERSION X.Y.Z ...)
cmake_version=$(grep -oP 'project\(PersistMemoryManager VERSION \K[0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt) || {
  echo "FAIL: could not extract version from CMakeLists.txt"
  exit 1
}

# README.md: badge like version-X.Y.Z-green
readme_version=$(grep -oP 'badge/version-\K[0-9]+\.[0-9]+\.[0-9]+' README.md) || {
  echo "FAIL: could not extract version badge from README.md"
  exit 1
}

# CHANGELOG.md: first ## [X.Y.Z] entry
changelog_version=$(grep -oP '^\#\# \[\K[0-9]+\.[0-9]+\.[0-9]+' CHANGELOG.md | head -1) || {
  echo "FAIL: could not extract latest version from CHANGELOG.md"
  exit 1
}

echo "CMakeLists.txt version: $cmake_version"
echo "README.md badge version: $readme_version"
echo "CHANGELOG.md latest version: $changelog_version"

# --- 2. Check consistency ---

if [ "$cmake_version" != "$readme_version" ]; then
  echo "FAIL: README badge version ($readme_version) does not match CMakeLists.txt ($cmake_version)"
  FAILED=1
fi

if [ "$cmake_version" != "$changelog_version" ]; then
  echo "FAIL: CHANGELOG latest version ($changelog_version) does not match CMakeLists.txt ($cmake_version)"
  FAILED=1
fi

# --- 3. Check that canonical docs listed in repo-policy.json exist ---

if [ -f repo-policy.json ]; then
  in_canonical=false
  while IFS= read -r line; do
    if echo "$line" | grep -q '"canonical_docs"'; then
      in_canonical=true
      continue
    fi
    if $in_canonical && echo "$line" | grep -q '^\s*\]'; then
      in_canonical=false
      continue
    fi
    if $in_canonical; then
      doc_path=$(echo "$line" | grep -oP '"[^"]*\.md"' | tr -d '"')
      if [ -n "$doc_path" ] && [ ! -f "$doc_path" ]; then
        echo "FAIL: canonical doc listed in repo-policy.json does not exist: $doc_path"
        FAILED=1
      fi
    fi
  done < repo-policy.json
fi

if [ "$FAILED" -eq 0 ]; then
  echo "OK: all docs-consistency checks passed"
else
  echo ""
  echo "Docs-consistency checks failed. Please fix the inconsistencies above."
fi

exit $FAILED
