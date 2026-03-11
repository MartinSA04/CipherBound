#!/usr/bin/env python3
"""
Converts 4 directional walk-cycle GIFs into a single spritesheet PNG
matching the player_sheet.png format used by the game.

Spritesheet layout (4x4 grid, each cell = frame_width x frame_height):
  Row 0 = down  (WalkS)
  Row 1 = up    (WalkN)
  Row 2 = left  (WalkW)
  Row 3 = right (WalkE)
  Columns 0-3 = walk cycle frames from the GIF

Usage:
  python3 make_spritesheet.py <gif_folder> [output.png]

Example:
  python3 make_spritesheet.py assets/sprites/npcs/bart_iver
  python3 make_spritesheet.py assets/sprites/npcs/bart_iver bart_iver_sheet.png
"""

import sys
import glob
import os
from PIL import Image


# Map direction suffix to spritesheet row index
DIR_ROW = {"S": 0, "N": 1, "W": 2, "E": 3}


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


def make_spritesheet(folder: str, output: str):
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


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <gif_folder> [output.png]")
        sys.exit(1)

    folder = sys.argv[1]
    if not os.path.isdir(folder):
        print(f"Error: {folder} is not a directory")
        sys.exit(1)

    # Default output: <folder_name>_sheet.png in the same parent directory
    if len(sys.argv) >= 3:
        output = sys.argv[2]
    else:
        name = os.path.basename(os.path.normpath(folder))
        output = os.path.join(folder, f"{name}_sheet.png")

    print(f"Creating spritesheet from {folder}")
    make_spritesheet(folder, output)


if __name__ == "__main__":
    main()
