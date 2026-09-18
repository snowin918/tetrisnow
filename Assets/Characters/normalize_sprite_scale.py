#!/usr/bin/env python3
"""Normalize character scale across every pose in the 7 in-game reaction
sheets, cell by cell, and balance the two characters against each other.

Each of the 112 cells (7 files x 2 characters x 8 poses) was drawn slightly
differently, so both switching emotion (a different file) and playing
through one animation (different columns in the same file) could make the
character visibly grow/shrink.

Content HEIGHT is the per-cell scaling metric: across a huge pose range
(standing to fully curled-up kneeling) it only drifts a few percent, unlike
width, which swings wildly with pose (e.g. lose.png's boy goes from 160px
wide standing to 266px wide curled up -- that's the pose, not a scale bug,
and forcing every cell's width to match would distort the art). Every cell
is scaled to hit a per-character height target, so both cross-file drift
(switching emotion) and within-file drift (playing one animation) are
caught, without introducing pose-driven jitter from width.

Height alone isn't enough to make the two characters look right side by
side, though: the girl's chibi design has a bigger head and a
wider/stockier body than the boy's slimmer one, so at matched height she
was measurably ~38% wider and visibly "bigger". BALANCE_METRIC fixes that
by comparing each character's REFERENCE_CELL (a neutral standing pose --
the col0 pose in every sheet, the same one relied on for the column-0
reasoning above) via sqrt(width*height), and folding a one-time correction
into that character's height target so their reference poses end up with
matched on-screen presence. This is deliberately based on the single
stable reference pose rather than an average over all (width-volatile)
poses, so the correction itself doesn't inherit width's pose-driven noise.

Run with no flags first to see the measured scale factors, then --apply to
write over the originals (backups are NOT made -- commit/copy first if you
want to compare against the current files).
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

SHEETS = ["idle.png", "attack1.png", "attack2.png", "damage1.png", "damage2.png", "win.png", "lose.png"]
N_ROWS = 4
N_COLS = 4
ALPHA_THRESH = 10
MARGIN_FRAC = 0.04
BOY_ROWS = (0, 1)
GIRL_ROWS = (2, 3)
REF_COL = 0


def cell_bbox(arr, x0, x1, y0, y1):
    band_alpha = arr[y0:y1, x0:x1, 3]
    ys, xs = np.where(band_alpha > ALPHA_THRESH)
    if len(ys) == 0:
        return None
    return int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1


def grid_geom(w, h):
    return w / N_COLS, h / N_ROWS


def measure_all_cells(arr, w, h):
    """Return {(row, col): (x0, y0, x1, y1, left, top, right, bottom, width, height)} for every cell."""
    cell_w, cell_h = grid_geom(w, h)
    cells = {}
    for row in range(N_ROWS):
        y0 = int(row * cell_h)
        y1 = int((row + 1) * cell_h) if row < N_ROWS - 1 else h
        for col in range(N_COLS):
            x0 = int(col * cell_w)
            x1 = int((col + 1) * cell_w) if col < N_COLS - 1 else w
            bbox = cell_bbox(arr, x0, x1, y0, y1)
            if bbox is None:
                continue
            left, top, right, bottom = bbox
            cells[(row, col)] = (x0, y0, x1, y1, left, top, right, bottom, right - left, bottom - top)
    return cells


def rescale_sheet(arr, w, h, cells, boy_target, girl_target):
    cell_w, cell_h = grid_geom(w, h)
    out_arr = np.zeros_like(arr)
    for (row, col), (x0, y0, x1, y1, left, top, right, bottom, content_w, content_h) in cells.items():
        target = boy_target if row in BOY_ROWS else girl_target
        scale = target / content_h

        margin_px = int((y1 - y0) * MARGIN_FRAC)
        avail_h = (y1 - y0) - 2 * margin_px

        content = Image.fromarray(arr[y0 + top:y0 + bottom, x0 + left:x0 + right])
        if abs(scale - 1.0) > 1e-3:
            new_w = max(1, int(content.width * scale))
            new_h = max(1, int(content.height * scale))
            if new_h > avail_h:
                shrink = avail_h / new_h
                new_w = max(1, int(new_w * shrink))
                new_h = max(1, int(new_h * shrink))
            content = content.resize((new_w, new_h), Image.LANCZOS)

        orig_center_x = x0 + left + (right - left) / 2.0
        paste_x = int(orig_center_x - content.width / 2.0)
        paste_y = y1 - margin_px - content.height

        # Clamp to the cell's own horizontal bounds (not just the whole
        # image) so a pose that got wider after an upscale can't bleed into
        # the neighboring column.
        paste_x = max(x0, min(paste_x, x1 - content.width))
        paste_x = max(0, min(paste_x, w - content.width))
        paste_y = max(y0, min(paste_y, h - content.height))

        content_arr = np.array(content)
        dst = out_arr[paste_y:paste_y + content.height, paste_x:paste_x + content.width]
        a = content_arr[:, :, 3:4].astype(np.float32) / 255.0
        dst[:] = (content_arr.astype(np.float32) * a + dst.astype(np.float32) * (1 - a)).astype(np.uint8)
        out_arr[paste_y:paste_y + content.height, paste_x:paste_x + content.width] = dst
    return out_arr


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true", help="Overwrite originals instead of writing *_scaled.png")
    parser.add_argument("--only", nargs="*", default=None)
    args = parser.parse_args()

    directory = Path(__file__).parent
    sheets = args.only if args.only else SHEETS

    loaded = {}
    all_boy_heights = []
    all_girl_heights = []
    boy_ref = []  # (width, height) of each file's col0 boy pose
    girl_ref = []
    for name in sheets:
        src = directory / name
        if not src.exists():
            print(f"skip (not found): {name}")
            continue
        im = Image.open(src).convert("RGBA")
        arr = np.array(im)
        w, h = im.size
        cells = measure_all_cells(arr, w, h)
        loaded[name] = (arr, w, h, cells)
        for (row, col), info in cells.items():
            content_w, content_h = info[-2], info[-1]
            (all_boy_heights if row in BOY_ROWS else all_girl_heights).append(content_h)
            if col == REF_COL:
                if row == BOY_ROWS[0]:
                    boy_ref.append((content_w, content_h))
                elif row == GIRL_ROWS[0]:
                    girl_ref.append((content_w, content_h))

    boy_height_target = float(np.median(all_boy_heights))
    girl_height_target = float(np.median(all_girl_heights))

    # Balance correction: compare the two characters' reference (col0,
    # standing) pose using sqrt(width*height), and fold a one-time uniform
    # correction into each height target so their reference poses end up
    # with matched on-screen presence.
    boy_ref_w = float(np.median([w for w, _ in boy_ref]))
    boy_ref_h = float(np.median([h for _, h in boy_ref]))
    girl_ref_w = float(np.median([w for w, _ in girl_ref]))
    girl_ref_h = float(np.median([h for _, h in girl_ref]))
    boy_ref_size = np.sqrt(boy_ref_w * boy_ref_h)
    girl_ref_size = np.sqrt(girl_ref_w * girl_ref_h)
    balanced_ref_size = np.sqrt(boy_ref_size * girl_ref_size)
    boy_correction = balanced_ref_size / boy_ref_size
    girl_correction = balanced_ref_size / girl_ref_size

    boy_target = boy_height_target * boy_correction
    girl_target = girl_height_target * girl_correction

    print(f"reference pose (col0): boy w={boy_ref_w:.0f} h={boy_ref_h:.0f}  girl w={girl_ref_w:.0f} h={girl_ref_h:.0f}")
    print(f"balance correction: boy x{boy_correction:.3f}  girl x{girl_correction:.3f}")
    print(f"height targets: boy {boy_height_target:.0f}px -> {boy_target:.0f}px, "
          f"girl {girl_height_target:.0f}px -> {girl_target:.0f}px")

    for name, (arr, w, h, cells) in loaded.items():
        src = directory / name
        heights = ", ".join(
            f"({r},{c})={info[-1]}" for (r, c), info in sorted(cells.items())
        )
        print(f"{name}: {heights}")

        out_arr = rescale_sheet(arr, w, h, cells, boy_target, girl_target)
        dst = src if args.apply else directory / f"{src.stem}_scaled.png"
        Image.fromarray(out_arr).save(dst)
        print(f"  wrote {dst.name}")


if __name__ == "__main__":
    sys.exit(main())
