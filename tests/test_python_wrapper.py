"""Tests for the ctypes Python wrapper around libgeodesicDT2d.

Requires the project to have been built first (`make`), which produces
libgeodesicDT2d.{so,dylib} alongside geodesicDT2d.py.
"""
import os
import sys

import pytest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, REPO_ROOT)

from geodesicDT2d import run_geodesic2dDT_wrapper  # noqa: E402

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    Image = None

DOMAIN = os.path.join(REPO_ROOT, "example_data", "domain256_1.png")
SOURCE_SINGLE = os.path.join(REPO_ROOT, "example_data", "source_2D_02.txt")
SOURCE_MULTI = os.path.join(REPO_ROOT, "example_data", "source_2D_4_points.txt")


def test_produces_nonempty_output(tmp_path):
    outfile = str(tmp_path / "out.png")
    run_geodesic2dDT_wrapper(SOURCE_SINGLE, DOMAIN, outfile, 0, 0)
    assert os.path.exists(outfile)
    assert os.path.getsize(outfile) > 0


def test_color_modes_all_produce_output(tmp_path):
    for color_mode in (0, 1, 2):
        outfile = str(tmp_path / f"out_{color_mode}.png")
        run_geodesic2dDT_wrapper(SOURCE_SINGLE, DOMAIN, outfile, color_mode, 0)
        assert os.path.getsize(outfile) > 0


@pytest.mark.skipif(Image is None, reason="Pillow not installed")
def test_output_dimensions_match_domain(tmp_path):
    outfile = str(tmp_path / "out.png")
    run_geodesic2dDT_wrapper(SOURCE_SINGLE, DOMAIN, outfile, 0, 0)
    with Image.open(DOMAIN) as domain_img, Image.open(outfile) as out_img:
        assert out_img.size == domain_img.size


@pytest.mark.skipif(Image is None, reason="Pillow not installed")
def test_distance_is_zero_at_each_source_point(tmp_path):
    # source_2D_4_points.txt places sources at (row, col) = (140, 234),
    # (22, 179), (211, 60) and (75, 181). The distance map is stored as
    # image[x, y] = image[col, row], and a point's own geodesic distance
    # to itself is always exactly zero, regardless of the domain shape.
    outfile = str(tmp_path / "multi.png")
    run_geodesic2dDT_wrapper(SOURCE_MULTI, DOMAIN, outfile, 0, 0)
    source_rows_cols = [(140, 234), (22, 179), (211, 60), (75, 181)]

    with Image.open(outfile) as img:
        for row, col in source_rows_cols:
            assert img.getpixel((col, row)) == 0
