#!/bin/bash
# Regenerate the before/after comparison GIFs used in the README.
#
# Usage: docs/examples/make_comparison_gifs.sh BASELINE_FINE FILTERED_FINE \
#        BASELINE_LARGE FILTERED_LARGE BASELINE_MIXED FILTERED_MIXED
#
# Each pair is a recording of the same sample with the filter disabled and
# enabled (see tmp/device-e2e/obs-sample-e2e.mjs). Output GIFs are written to
# docs/examples/{fine,large,mixed}-comparison.gif.
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
OUT_DIR="${SCRIPT_DIR}"

if [ "$#" -ne 6 ]; then
  printf 'Usage: %s BASE_FINE FILT_FINE BASE_LARGE FILT_LARGE BASE_MIXED FILT_MIXED\n' "$0" >&2
  exit 1
fi

make_gif() {
  local base="$1" filtered="$2" out="$3"
  ffmpeg -y -v error -ss 2 -t 4 -i "$base" -ss 2 -t 4 -i "$filtered" \
    -i "${SCRIPT_DIR}/labels/no-filter.png" -i "${SCRIPT_DIR}/labels/stabilizer.png" \
    -filter_complex \
    "[0:v]scale=320:180:flags=bilinear[left];\
     [1:v]scale=320:180:flags=bilinear[right];\
     [left][2:v]overlay=8:8[l2];[right][3:v]overlay=8:8[r2];\
     [l2][r2]hstack=2,fps=8,split[a][b];[a]palettegen=stats_mode=diff[p];[b][p]paletteuse=dither=bayer" \
    "$out"
  printf 'wrote %s\n' "$out"
}

make_gif "$1" "$2" "${OUT_DIR}/fine-comparison.gif"
make_gif "$3" "$4" "${OUT_DIR}/large-comparison.gif"
make_gif "$5" "$6" "${OUT_DIR}/mixed-comparison.gif"
