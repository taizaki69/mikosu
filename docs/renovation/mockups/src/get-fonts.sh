#!/usr/bin/env bash
# Downloads the OFL fonts the mockups use (Google Fonts repo) into ./fonts (git-ignored).
set -euo pipefail
cd "$(dirname "$0")" && mkdir -p fonts && cd fonts
base=https://raw.githubusercontent.com/google/fonts/main/ofl
curl -fsSL -o Nunito-wght.ttf "$base/nunito/Nunito%5Bwght%5D.ttf"
curl -fsSL -o Nunito-Italic-wght.ttf "$base/nunito/Nunito-Italic%5Bwght%5D.ttf"
curl -fsSL -o Nunito-OFL.txt "$base/nunito/OFL.txt"
curl -fsSL -o MPLUSRounded1c-Regular.ttf "$base/mplusrounded1c/MPLUSRounded1c-Regular.ttf"
curl -fsSL -o MPLUSRounded1c-Bold.ttf "$base/mplusrounded1c/MPLUSRounded1c-Bold.ttf"
