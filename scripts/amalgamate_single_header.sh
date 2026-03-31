#!/usr/bin/env sh
# Regenerate single_header/linq.h (requires python3)
set -e
cd "$(dirname "$0")/.."
exec python3 scripts/amalgamate_single_header.py
