#!/usr/bin/env python3
"""Re-fit each pose in a 4x4 character sprite sheet into its own cell.

The generated sheets (idle.png, attack1.png, ...) don't strictly respect
their 4x4 grid: a pose's feet/hair sometimes extends past its cell into the
neighboring row, which bleeds into that neighbor's crop at render time.

This script, per column:
  1. Finds the low-density gaps between the 4 stacked poses (the same
     technique used to diagnose the bug: alpha pixel count per row, look for
     the local minimum near each nominal row boundary).
  2. Crops each pose to its true (gap-to-gap) content bounding box.
  3. Scales it down only if it's taller than its nominal cell (minus margin),
     then pastes it back centered in that nominal cell.

Run with --dry-run first to inspect the *_fixed.png outputs before
overwriting the originals with --apply.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

SHEETS = ["idle.png", "attack1.png", "attack2.png", "damage1.png", "damage2.png", "win.png", "lose.png"]
N_ROWS = 4
N_COLS = 4
MARGIN_FRAC = 0.04  # blank margin kept at top/bottom of each cell
ALPHA_THRESH = 10
DENSITY_MIN_FRAC = 0.0  # not currently used; kept for tuning
SEARCH_BEFORE = 60  # px to search before a nominal boundary for the true gap
SEARCH_AFTER = 100  # px to search after a nominal boundary for the true gap


def find_row_splits(alpha_col, cell_h, n_rows):
    """Return n_rows+1 y-splits bounding each stacked pose in one column."""
    h = alpha_col.shape[0]
    density = (alpha_col > ALPHA_THRESH).sum(axis=1)
    splits = [0]
    for r in range(1, n_rows):
        boundary = int(r * cell_h)
        lo = max(0, boundary - SEARCH_BEFORE)
        hi = min(h, boundary + SEARCH_AFTER)
        window = density[lo:hi]
        if len(window) == 0:
            splits.append(boundary)
            continue
        min_idx = int(np.argmin(window))
        splits.append(lo + min_idx)
    splits.append(h)
    return splits


def fix_sheet(path: Path, out_path: Path, margin_frac=MARGIN_FRAC):
    im = Image.open(path).convert("RGBA")
    arr = np.array(im)
    h, w = arr.shape[:2]
    cell_w = w / N_COLS
    cell_h = h / N_ROWS
    out_arr = np.zeros_like(arr)

    for col in range(N_COLS):
        x0 = int(col * cell_w)
        x1 = int((col + 1) * cell_w) if col < N_COLS - 1 else w
        col_alpha = arr[:, x0:x1, 3]
        splits = find_row_splits(col_alpha, cell_h, N_ROWS)

        for r in range(N_ROWS):
            top, bottom = splits[r], splits[r + 1]
            if bottom <= top:
                continue
            band = arr[top:bottom, x0:x1]
            band_alpha = band[:, :, 3]
            ys, xs = np.where(band_alpha > ALPHA_THRESH)
            if len(ys) == 0:
                continue
            pad = 2
            c_top = max(0, int(ys.min()) - pad)
            c_bottom = min(band.shape[0], int(ys.max()) + pad + 1)
            c_left = max(0, int(xs.min()) - pad)
            c_right = min(band.shape[1], int(xs.max()) + pad + 1)
            content = Image.fromarray(band[c_top:c_bottom, c_left:c_right])

            nominal_top = int(r * cell_h)
            nominal_bottom = int((r + 1) * cell_h) if r < N_ROWS - 1 else h
            nominal_h = nominal_bottom - nominal_top
            margin_px = int(nominal_h * margin_frac)
            avail_h = nominal_h - 2 * margin_px

            scale = min(1.0, avail_h / content.height) if content.height > 0 else 1.0
            if scale < 1.0:
                new_w = max(1, int(content.width * scale))
                new_h = max(1, int(content.height * scale))
                content = content.resize((new_w, new_h), Image.LANCZOS)

            paste_x = x0 + c_left
            # Anchor to the cell's bottom margin (feet on the ground), matching
            # how these sheets are meant to line up ("foot baseline" convention).
            paste_y = nominal_bottom - margin_px - content.height
            paste_y = max(nominal_top, paste_y)

            paste_x = max(0, min(paste_x, w - content.width))
            paste_y = max(0, min(paste_y, h - content.height))

            content_arr = np.array(content)
            dst = out_arr[paste_y:paste_y + content.height, paste_x:paste_x + content.width]
            src_alpha = content_arr[:, :, 3:4].astype(np.float32) / 255.0
            dst[:] = (content_arr.astype(np.float32) * src_alpha + dst.astype(np.float32) * (1 - src_alpha)).astype(np.uint8)
            out_arr[paste_y:paste_y + content.height, paste_x:paste_x + content.width] = dst

    Image.fromarray(out_arr).save(out_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true", help="Overwrite the original files instead of writing *_fixed.png")
    parser.add_argument("--only", nargs="*", default=None, help="Subset of sheet filenames to process")
    args = parser.parse_args()

    directory = Path(__file__).parent
    sheets = args.only if args.only else SHEETS
    for name in sheets:
        src = directory / name
        if not src.exists():
            print(f"skip (not found): {name}")
            continue
        dst = src if args.apply else directory / f"{src.stem}_fixed.png"
        fix_sheet(src, dst)
        print(f"wrote {dst.name}")


if __name__ == "__main__":
    sys.exit(main())
