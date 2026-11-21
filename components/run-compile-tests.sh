#!/usr/bin/env bash
# Run the CI compile tests locally (macOS / Linux). Windows: run-compile-tests.ps1
#
#   components/run-compile-tests.sh                 # ESPHome on your PATH
#   components/run-compile-tests.sh min             # minimum version from the CI workflow
#   components/run-compile-tests.sh 2026.10.0       # any other version
#   components/run-compile-tests.sh min levoit      # only some components
#
# A pinned version is installed once into .venv-esphome-<version>/ at the repo
# root (git-ignored), so your normal ESPHome install is not touched.
# Full build logs: components/<component>/tests/.esphome/<config>.log
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORKFLOW="$ROOT/.github/workflows/compile-tests.yml"
# Every components/<component>/tests/test-*.yaml is built, so a new test
# config (e.g. one that only exists for levoit) needs no change here.

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    echo "Git Bash / MSYS skips the ESP-IDF build while ESPHome still reports success."
    echo "Use PowerShell instead: components\\run-compile-tests.ps1"
    exit 1 ;;
esac

VERSION="${1:-}"
[ $# -gt 0 ] && shift
COMPONENTS="${*:-levoit philips}"

if [ "$VERSION" = "min" ]; then
  VERSION="$(grep -E '^[[:space:]]*esphome:[[:space:]]*\[' "$WORKFLOW" | grep -oE '[0-9]{4}\.[0-9]+\.[0-9]+' | head -1)"
  if [ -z "$VERSION" ]; then
    echo "Could not read the minimum ESPHome version from $WORKFLOW"; exit 1
  fi
fi

if [ -n "$VERSION" ] && [ "$VERSION" != "latest" ]; then
  VENV="$ROOT/.venv-esphome-$VERSION"
  if [ ! -x "$VENV/bin/esphome" ]; then
    echo "Installing ESPHome $VERSION into $VENV ..."
    python3 -m venv "$VENV" && "$VENV/bin/pip" install --quiet "esphome==$VERSION" || exit 1
  fi
  ESPHOME="$VENV/bin/esphome"
else
  ESPHOME="esphome"
fi

VERSION_STR="$("$ESPHOME" version)"
echo "Using $VERSION_STR"
FAILED=0
for c in $COMPONENTS; do
  dir="$ROOT/components/$c/tests"
  mkdir -p "$dir/.esphome"
  # A build folder left by another ESPHome version breaks the link (stale
  # generated files), so start clean whenever the version changes.
  marker="$dir/.esphome/esphome-version"
  last="$(cat "$marker" 2>/dev/null || true)"
  if [ "$last" != "$VERSION_STR" ] && [ -d "$dir/.esphome/build" ]; then
    echo "$c: ESPHome version changed (${last:-unknown} -> $VERSION_STR), removing generated $dir/.esphome/build"
    rm -rf "$dir/.esphome/build"
  fi
  echo "$VERSION_STR" > "$marker"
  for cfg in "$dir"/test-*.yaml; do
    [ -e "$cfg" ] || continue
    t="$(basename "$cfg" .yaml)"
    log="$dir/.esphome/$t.log"
    printf '%-8s %-24s ' "$c" "$t"
    if (cd "$dir" && "$ESPHOME" compile "$t.yaml") > "$log" 2>&1; then
      echo "PASS"
    else
      echo "FAIL  (log: ${log#$ROOT/})"
      grep -m 5 "error:" "$log" | sed 's/^/    /'
      FAILED=$((FAILED + 1))
    fi
  done
done

[ "$FAILED" -eq 0 ] && echo "All passed." || echo "$FAILED failed."
exit "$FAILED"
