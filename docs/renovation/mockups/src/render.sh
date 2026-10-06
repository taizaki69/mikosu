#!/usr/bin/env bash
# Renders mockups (screen x variant) to ../<screen>-<variant>.jpg with headless Firefox at 1920x1080.
# Needs: fonts (./get-fonts.sh), art (python3 make_art.py), firefox.
set -euo pipefail
cd "$(dirname "$0")"
here="$(pwd)"
profile="$(mktemp -d)"
trap 'rm -rf "$profile"' EXIT
# round 1: SCREENS="mainmenu songselect" VARIANTS="faithful refined bold"
# round 2: SCREENS="mainmenu2 songselect2" VARIANTS="all controls accents"
# round 3: SCREENS="mainmenu3 songselect3" VARIANTS="lazer tinted upright"
for screen in ${SCREENS:-mainmenu3 songselect3}; do
  for v in ${VARIANTS:-lazer tinted upright}; do
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
