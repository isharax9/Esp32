#!/usr/bin/env bash
set -e

# Fast installer forwarding to android_app/scripts/install.sh
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec "$PROJECT_DIR/android_app/scripts/install.sh" "$@"
