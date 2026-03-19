#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build-clang"
REPO_ROOT="$(pwd)"
SCAN_REPORT_DIR="${BUILD_DIR}/scan-build-reports"
SCAN_LOG="${BUILD_DIR}/scan-build.log"
TIDY_LOG="${BUILD_DIR}/clang-tidy.log"

log() {
  echo "[clang-analysis] $*"
}

require_cmd() {
  local cmd="$1"
  if ! command -v "$cmd" >/dev/null 2>&1; then
    log "ERROR: required command not found: $cmd"
    exit 1
  fi
}

log "Checking required tooling"
require_cmd cmake
require_cmd clang
require_cmd clang++
require_cmd scan-build
require_cmd clang-tidy
require_cmd python3

log "Configuring CMake in ${BUILD_DIR}"
cmake -S . -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

log "Building CanModuleMain once without analyzers (prebuild external dependencies)"
cmake --build "$BUILD_DIR" --config Release --target CanModuleMain

log "Marking project sources as modified for analyzer-only rebuild"
find src/main src/python src/include -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hpp' -o -name '*.hxx' \) -exec touch {} +

log "Running Clang Static Analyzer (scan-build)"
rm -rf "$SCAN_REPORT_DIR"
mkdir -p "$SCAN_REPORT_DIR"
set +e
scan-build --keep-empty -plist -o "$SCAN_REPORT_DIR" --use-cc=clang --use-c++=clang++ \
  cmake --build "$BUILD_DIR" --config Release --target CanModuleMain |& tee "$SCAN_LOG"
scan_build_status=${PIPESTATUS[0]}
set -e

scan_summary=$(
python3 - "$SCAN_REPORT_DIR" "$REPO_ROOT" <<'PY'
import pathlib
import plistlib
import sys

report_root = pathlib.Path(sys.argv[1])
repo_root = pathlib.Path(sys.argv[2]).resolve()
hits = []

for plist_path in report_root.rglob("*.plist"):
    try:
        data = plistlib.loads(plist_path.read_bytes())
    except Exception:
        continue

    files = data.get("files", [])
    diagnostics = data.get("diagnostics", [])
    for d in diagnostics:
        loc = d.get("location", {})
        file_idx = loc.get("file")
        if not isinstance(file_idx, int) or file_idx < 0 or file_idx >= len(files):
            continue

        raw_path = pathlib.Path(files[file_idx])
        abs_path = raw_path if raw_path.is_absolute() else (repo_root / raw_path)
        abs_path = abs_path.resolve()
        try:
            rel_path = abs_path.relative_to(repo_root)
        except Exception:
            continue

        rel = str(rel_path)
        if not rel.startswith("src/"):
            continue

        line = loc.get("line", 0)
        col = loc.get("col", 0)
        checker = d.get("check_name", "unknown-checker")
        desc = d.get("description", "").replace("\n", " ").strip()
        hits.append((rel, line, col, checker, desc))

for rel, line, col, checker, desc in hits:
    print(f"{rel}:{line}:{col}: [{checker}] {desc}")

print(f"SCAN_PROJECT_COUNT={len(hits)}")
PY
)
echo "$scan_summary"
scan_project_count="$(echo "$scan_summary" | awk -F= '/^SCAN_PROJECT_COUNT=/{print $2}' | tail -n1)"
scan_project_count="${scan_project_count:-0}"
scan_completed=0
if grep -q "scan-build: Analysis run complete." "$SCAN_LOG"; then
  scan_completed=1
fi
scan_invocation_failed=0

if [[ $scan_build_status -ne 0 && $scan_completed -eq 0 ]]; then
  scan_invocation_failed=1
  log "ERROR: scan-build build invocation failed (exit=${scan_build_status})"
fi

log "Running clang-tidy"
tidy_status=0
rm -f "$TIDY_LOG"
mapfile -t tidy_files < <(find src/main src/python -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) | sort)
if [[ ${#tidy_files[@]} -eq 0 ]]; then
  log "ERROR: no project C/C++ source files found for clang-tidy"
  exit 1
fi

for f in "${tidy_files[@]}"; do
  set +e
  clang-tidy -p "$BUILD_DIR" --warnings-as-errors='*' "$f" |& tee -a "$TIDY_LOG"
  file_status=${PIPESTATUS[0]}
  set -e
  if [[ $file_status -ne 0 ]]; then
    tidy_status=1
  fi
done

if [[ $tidy_status -ne 0 ]]; then
  log "clang-tidy findings (project sources only):"
  grep -E '^.*/src/.*:[0-9]+:[0-9]+: (warning|error):' "$TIDY_LOG" || true
fi

if [[ $scan_invocation_failed -ne 0 || $scan_project_count -ne 0 || $tidy_status -ne 0 ]]; then
  log "ERROR: analyzer failures detected (scan-invocation-failed=$scan_invocation_failed, scan-build-exit=$scan_build_status, scan-project-findings=$scan_project_count, clang-tidy=$tidy_status)"
  exit 1
fi

log "Clang analysis completed without findings"
