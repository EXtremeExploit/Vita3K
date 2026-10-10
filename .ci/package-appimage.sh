#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/common.sh"

PRESET=""
CONFIG=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --preset)
            PRESET="$2"
            shift 2
            ;;
        --config)
            CONFIG="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

require_arg "$PRESET" "preset"
require_arg "$CONFIG" "config"

case "$(uname -m)" in
    x86_64|aarch64)
        ARCH="$(uname -m)"
        ;;
    *)
        echo "Unsupported AppImage architecture: $(uname -m)" >&2
        exit 1
        ;;
esac

INSTALL_PREFIX="/usr"
OUTPUT_DIR="build/$PRESET/bin/$CONFIG"
QUICK_SHARUN_URL="https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/refs/heads/main/useful-tools/quick-sharun.sh"
TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT
QUICK_SHARUN="$TEMP_DIR/quick-sharun.sh"
APPDIR="$TEMP_DIR/AppDir"

sudo cmake --install "build/$PRESET" --config "$CONFIG" --prefix "$INSTALL_PREFIX"
curl -fsSL --retry 3 --retry-delay 10 "$QUICK_SHARUN_URL" -o "$QUICK_SHARUN"
chmod +x "$QUICK_SHARUN"

export APPDIR
export DESKTOP="$INSTALL_PREFIX/share/applications/org.vita3k.vita3k.desktop"
export ICON="$INSTALL_PREFIX/share/icons/hicolor/scalable/apps/org.vita3k.vita3k.svg"
export OUTPATH="$OUTPUT_DIR"
export OUTNAME="Vita3K-$ARCH.AppImage"
export UPINFO="gh-releases-zsync|Vita3K|Vita3K|continuous|Vita3K-$ARCH.AppImage.zsync"
export OUTPUT_APPIMAGE=0
if [[ -n "${QT_ROOT_DIR:-}" ]]; then
    export QT_LOCATION="$QT_ROOT_DIR"
fi

"$QUICK_SHARUN" "$INSTALL_PREFIX/bin/Vita3K"
test -d "$APPDIR/share/Vita3K"
"$QUICK_SHARUN" --make-appimage