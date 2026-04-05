#!/usr/bin/env python3
"""
Converts either:
1. 4 directional walk-cycle GIFs in a folder, or
2. a single 3x4 tile NPC sprite region,
into a 4x4 spritesheet PNG matching the game's overworld format.

Output spritesheet layout (4x4 grid):
  Row 0 = down
  Row 1 = up
  Row 2 = left
  Row 3 = right
  Columns 0-3 = idle, walk1, idle, walk2

Single-region input layout (3x4 grid of 32x32 tiles):
  Row 0: idle up,    idle right,  walk up1
  Row 1: walk left1, walk right1, idle down
  Row 2: idle left,  walk right2, walk down1
  Row 3: walk left2, walk up2,    walk down2

Usage:
  python3 tools/make_spritesheet.py <gif_folder_or_region.png> [output.png]

Examples:
  python3 tools/make_spritesheet.py assets/sprites/npcs/bart_iver
  python3 tools/make_spritesheet.py npc_region.png
  python3 tools/make_spritesheet.py npc_region.png output_sheet.png
"""

import argparse
import glob
import os
from pathlib import Path

from PIL import Image


DIR_ROW = {"S": 0, "N": 1, "W": 2, "E": 3}
TILE_SIZE = 32
REGION_COLS = 3
REGION_ROWS = 4

REGION_TO_SHEET = {
    "down": ((2, 1), (2, 2), (2, 1), (2, 3)),
    "up": ((0, 0), (2, 0), (0, 0), (1, 3)),
    "left": ((0, 2), (0, 1), (0, 2), (0, 3)),
    "right": ((1, 0), (1, 1), (1, 0), (1, 2)),
}

SHEET_ROW = {"down": 0, "up": 1, "left": 2, "right": 3}


def find_gif(folder: str, direction: str) -> str:
    """Find the GIF file for a given direction (S/N/W/E) in the folder."""
    pattern = os.path.join(folder, f"*Walk{direction}*")
    matches = glob.glob(pattern)
    if not matches:
        raise FileNotFoundError(f"No GIF found for Walk{direction} in {folder}")
    return matches[0]


def extract_frames(gif_path: str) -> list[Image.Image]:
    """Extract all frames from a GIF as RGBA images."""
    gif = Image.open(gif_path)
    frames = []
    for i in range(gif.n_frames):
        gif.seek(i)
        frames.append(gif.convert("RGBA"))
    return frames


def make_spritesheet_from_gifs(folder: str, output: str) -> None:
    # Load all directions
    directions = {}
    for d in DIR_ROW:
        gif_path = find_gif(folder, d)
        directions[d] = extract_frames(gif_path)
        print(f"  {d}: {gif_path} ({len(directions[d])} frames)")

    # Determine frame size from the first frame
    fw, fh = directions["S"][0].size
    num_cols = max(len(frames) for frames in directions.values())

    print(f"  Frame size: {fw}x{fh}, columns: {num_cols}")

    # Create the spritesheet
    sheet = Image.new("RGBA", (fw * num_cols, fh * 4), (0, 0, 0, 0))

    for d, row in DIR_ROW.items():
        frames = directions[d]
        for col, frame in enumerate(frames):
            # Resize if frame doesn't match expected size
            if frame.size != (fw, fh):
                frame = frame.resize((fw, fh), Image.NEAREST)
            sheet.paste(frame, (col * fw, row * fh))

    sheet.save(output)
    print(f"  Saved: {output} ({sheet.size[0]}x{sheet.size[1]})")


def extract_tile(region: Image.Image, tile_x: int, tile_y: int) -> Image.Image:
    left = tile_x * TILE_SIZE
    top = tile_y * TILE_SIZE
    return region.crop((left, top, left + TILE_SIZE, top + TILE_SIZE))


def make_spritesheet_from_region(image_path: str, output: str) -> None:
    region = Image.open(image_path).convert("RGBA")
    expected_size = (REGION_COLS * TILE_SIZE, REGION_ROWS * TILE_SIZE)
    if region.size != expected_size:
        raise ValueError(
            f"Expected a single 3x4 region of size {expected_size[0]}x{expected_size[1]}, "
            f"got {region.size[0]}x{region.size[1]}"
        )

    sheet = Image.new("RGBA", (TILE_SIZE * 4, TILE_SIZE * 4), (0, 0, 0, 0))
    for direction, frames in REGION_TO_SHEET.items():
        row = SHEET_ROW[direction]
        for col, (tile_x, tile_y) in enumerate(frames):
            sheet.paste(extract_tile(region, tile_x, tile_y), (col * TILE_SIZE, row * TILE_SIZE))

    sheet.save(output)
    print(f"  Saved: {output} ({sheet.size[0]}x{sheet.size[1]})")


def default_output_path(input_path: str) -> str:
    path = Path(input_path)
    if path.is_dir():
        return str(path / f"{path.name}_sheet.png")
    return str(path.with_name(f"{path.stem}_sheet.png"))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a game-format NPC spritesheet from GIFs or a single 3x4 region."
    )
    parser.add_argument("input", help="GIF folder or single-region PNG")
    parser.add_argument("output", nargs="?", help="Output spritesheet path")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    input_path = args.input
    output = args.output or default_output_path(input_path)

    if os.path.isdir(input_path):
        print(f"Creating spritesheet from GIF folder {input_path}")
        make_spritesheet_from_gifs(input_path, output)
        return

    if os.path.isfile(input_path):
        print(f"Creating spritesheet from single region {input_path}")
        make_spritesheet_from_region(input_path, output)
        return

    raise FileNotFoundError(f"Input path does not exist: {input_path}")


if __name__ == "__main__":
    main()
