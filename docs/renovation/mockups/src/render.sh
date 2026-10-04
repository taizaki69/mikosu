#!/usr/bin/env bash
# Renders every mockup (screen x variant) to ../<screen>-<variant>.png with headless Firefox at 1920x1080.
# Needs: fonts (./get-fonts.sh), art (python3 make_art.py), firefox.
set -euo pipefail
cd "$(dirname "$0")"
here="$(pwd)"
profile="$(mktemp -d)"
trap 'rm -rf "$profile"' EXIT
for screen in ${SCREENS:-mainmenu songselect}; do
  for v in faithful refined bold; do
    out="$here/../$screen-$v.png"
    rm -f "$out"
    firefox --headless --no-remote --profile "$profile" --window-size=1920,1080 \
      --screenshot "$out" "file://$here/$screen.html?v=$v" >/dev/null 2>&1
    # the repo keeps compact JPEGs
    python3 -c "from PIL import Image; import sys; Image.open(sys.argv[1]).convert('RGB').save(sys.argv[2], 'JPEG', quality=88, optimize=True, progressive=True)" "$out" "${out%.png}.jpg"
    rm -f "$out"
    echo "rendered $(basename "${out%.png}.jpg")"
  done
done
