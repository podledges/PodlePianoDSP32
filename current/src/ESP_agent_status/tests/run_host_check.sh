#!/usr/bin/env sh
set -eu

build_dir="${TMPDIR:-/tmp}/esp-agent-status-test-$$"
trap 'rm -rf "$build_dir"' EXIT
mkdir -p "$build_dir"

${CC:-cc} -std=c11 -Wall -Wextra -Werror \
  -I../main ../main/status_model.c status_model_test.c \
  -o "$build_dir/status_model_test"
"$build_dir/status_model_test"
