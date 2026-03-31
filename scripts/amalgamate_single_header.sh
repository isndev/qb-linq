#!/bin/sh
# Regenerate single_header/linq.h (requires python3). Prefer: python3 scripts/amalgamate_single_header.py
set -e
cd "$(dirname "$0")/.."
exec python3 scripts/amalgamate_single_header.py
