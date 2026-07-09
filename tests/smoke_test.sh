#!/usr/bin/env bash
# End-to-end smoke test: builds the CLI tools and runs them against the
# example data, checking that valid non-empty outputs are produced.
set -euo pipefail
cd "$(dirname "$0")/.."

OUTDIR=$(mktemp -d)
trap 'rm -rf "$OUTDIR"' EXIT

echo "== 2D smoke test =="
./geodesicDT2d -c 0 example_data/source_2D_02.txt example_data/domain256_1.png "$OUTDIR/out2d.png"
test -s "$OUTDIR/out2d.png"
echo "OK: $OUTDIR/out2d.png produced"

echo "== 2D multi-source smoke test =="
./geodesicDT2d -c 2 example_data/source_2D_4_points.txt example_data/domain256_1.png "$OUTDIR/out2d_multi.png"
test -s "$OUTDIR/out2d_multi.png"
echo "OK: $OUTDIR/out2d_multi.png produced"

echo "== 3D smoke test =="
./geodesicDT3d example_data/source_3D_01.txt example_data/domain3D_80_01.mhd "$OUTDIR/out3d.mhd"
test -s "$OUTDIR/out3d.mhd"
test -s "$OUTDIR/out3d.raw"
echo "OK: $OUTDIR/out3d.mhd + .raw produced"

echo "All smoke tests passed"
