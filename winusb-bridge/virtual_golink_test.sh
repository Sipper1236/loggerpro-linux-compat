#!/usr/bin/env bash
set -euo pipefail

source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir="$source_dir/build"

"$source_dir/build.sh" >/dev/null
"$build_dir/virtual_golink_uhid" --self-test
