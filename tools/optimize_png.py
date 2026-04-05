#!/usr/bin/env python3
"""
Losslessly optimize PNG files by trying smaller encodings such as:
- palette / indexed PNG when the image uses <= 256 RGBA colors
- grayscale / grayscale+alpha when possible
- RGB / RGBA re-encoding with max compression and stripped metadata

Every candidate is verified by decoding it again and comparing RGBA pixels to the source.

Usage examples:
  python3 tools/optimize_png.py assets/sprites/foo.png --in-place
  python3 tools/optimize_png.py assets/sprites assets/tilesets --in-place
  python3 tools/optimize_png.py assets/sprites --recursive --verbose
"""

from __future__ import annotations

import argparse
import io
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from PIL import Image


PNG_SUFFIX = ".png"


@dataclass(frozen=True)
class OptimizationResult:
    source: Path
    output: Path
    strategy: str
    original_size: int
    optimized_size: int
    changed: bool

    @property
    def saved_bytes(self) -> int:
        return self.original_size - self.optimized_size


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Losslessly optimize PNG files.")
    parser.add_argument("paths", nargs="+", help="PNG files or directories to process")
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Overwrite each source PNG with the optimized result",
    )
    parser.add_argument(
        "--output-dir",
        help="Write optimized files to this directory, preserving relative names where possible",
    )
    parser.add_argument(
        "--recursive",
        action="store_true",
        help="Recurse into directories. Default: only direct child PNGs.",
    )
    parser.add_argument(
        "--suffix",
        default="_optimized",
        help="Suffix for output files when not using --in-place or --output-dir",
    )
    parser.add_argument(
        "--keep-larger",
        action="store_true",
        help="Still write output even when no smaller lossless encoding is found",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print the chosen strategy for each file",
    )
    return parser.parse_args()


def iter_pngs(paths: Iterable[str], recursive: bool) -> list[Path]:
    found: list[Path] = []
    for raw in paths:
        path = Path(raw)
        if path.is_file():
            if path.suffix.lower() == PNG_SUFFIX:
                found.append(path)
            continue

        if not path.is_dir():
            raise FileNotFoundError(f"Path does not exist: {path}")

        pattern = "**/*.png" if recursive else "*.png"
        found.extend(sorted(child for child in path.glob(pattern) if child.is_file()))

    unique: dict[Path, None] = {}
    for path in found:
        unique[path] = None
    return list(unique.keys())


def rgba_image(path: Path) -> Image.Image:
    with Image.open(path) as image:
        return image.convert("RGBA")


def rgba_signature(image: Image.Image) -> tuple[tuple[int, int], bytes]:
    rgba = image.convert("RGBA")
    return rgba.size, rgba.tobytes()


def iter_rgba_pixels(image: Image.Image) -> Iterable[tuple[int, int, int, int]]:
    raw = image.convert("RGBA").tobytes()
    for i in range(0, len(raw), 4):
        yield (raw[i], raw[i + 1], raw[i + 2], raw[i + 3])


def save_png_bytes(image: Image.Image, *, transparency: bytes | None = None) -> bytes:
    buffer = io.BytesIO()
    save_kwargs: dict[str, object] = {"format": "PNG", "optimize": True, "compress_level": 9}
    if transparency is not None:
        save_kwargs["transparency"] = transparency
    image.save(buffer, **save_kwargs)
    return buffer.getvalue()


def verify_lossless(source_rgba: Image.Image, candidate_bytes: bytes) -> bool:
    source_size, source_bytes = rgba_signature(source_rgba)
    with Image.open(io.BytesIO(candidate_bytes)) as candidate:
        candidate_size, candidate_rgba = rgba_signature(candidate)
    return source_size == candidate_size and source_bytes == candidate_rgba


def can_use_grayscale(rgba: Image.Image) -> tuple[bool, bool]:
    has_alpha = False
    for red, green, blue, alpha in iter_rgba_pixels(rgba):
        if red != green or red != blue:
            return False, False
        if alpha != 255:
            has_alpha = True
    return True, has_alpha


def build_palette_candidate(rgba: Image.Image) -> bytes | None:
    colors = rgba.getcolors(maxcolors=257)
    if colors is None:
        return None

    palette_map: dict[tuple[int, int, int, int], int] = {}
    palette_rgb: list[int] = []
    transparency: list[int] = []
    indices: list[int] = []

    for pixel in iter_rgba_pixels(rgba):
        index = palette_map.get(pixel)
        if index is None:
            index = len(palette_map)
            palette_map[pixel] = index
            palette_rgb.extend(pixel[:3])
            transparency.append(pixel[3])
        indices.append(index)

    paletted = Image.new("P", rgba.size)
    paletted.putdata(indices)
    paletted.putpalette(palette_rgb + [0] * (256 * 3 - len(palette_rgb)))

    transparency_bytes = None
    if any(alpha != 255 for alpha in transparency):
        transparency_bytes = bytes(transparency)

    return save_png_bytes(paletted, transparency=transparency_bytes)


def build_gray_candidate(rgba: Image.Image, *, with_alpha: bool) -> bytes:
    mode = "LA" if with_alpha else "L"
    return save_png_bytes(rgba.convert(mode))


def build_direct_candidate(rgba: Image.Image, *, opaque: bool) -> bytes:
    return save_png_bytes(rgba.convert("RGB" if opaque else "RGBA"))


def choose_best_candidate(path: Path) -> tuple[bytes, str, int]:
    original_bytes = path.read_bytes()
    rgba = rgba_image(path)

    candidates: list[tuple[str, bytes]] = [("original", original_bytes)]
    is_gray, has_gray_alpha = can_use_grayscale(rgba)
    if is_gray:
        candidates.append(("grayscale_alpha" if has_gray_alpha else "grayscale",
                           build_gray_candidate(rgba, with_alpha=has_gray_alpha)))

    opaque = all(alpha == 255 for _, _, _, alpha in iter_rgba_pixels(rgba))
    candidates.append(("rgb" if opaque else "rgba", build_direct_candidate(rgba, opaque=opaque)))

    palette_candidate = build_palette_candidate(rgba)
    if palette_candidate is not None:
        candidates.append(("indexed", palette_candidate))

    valid_candidates: list[tuple[str, bytes]] = []
    for strategy, candidate in candidates:
        if verify_lossless(rgba, candidate):
            valid_candidates.append((strategy, candidate))

    if not valid_candidates:
        raise RuntimeError(f"No verified lossless candidate generated for {path}")

    best_strategy, best_bytes = min(valid_candidates, key=lambda item: len(item[1]))
    return best_bytes, best_strategy, len(original_bytes)


def build_output_path(source: Path, args: argparse.Namespace) -> Path:
    if args.in_place:
        return source

    if args.output_dir:
        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        return output_dir / source.name

    return source.with_name(f"{source.stem}{args.suffix}{source.suffix}")


def optimize_one(path: Path, args: argparse.Namespace) -> OptimizationResult:
    optimized_bytes, strategy, original_size = choose_best_candidate(path)
    output_path = build_output_path(path, args)
    changed = len(optimized_bytes) < original_size

    if changed or args.keep_larger or args.in_place:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        if changed or output_path != path or args.keep_larger:
            output_path.write_bytes(optimized_bytes if changed or args.keep_larger else path.read_bytes())

    return OptimizationResult(
        source=path,
        output=output_path,
        strategy=strategy,
        original_size=original_size,
        optimized_size=len(optimized_bytes),
        changed=changed,
    )


def format_result(result: OptimizationResult, verbose: bool) -> str:
    if result.changed:
        message = (
            f"{result.source} -> {result.output} "
            f"saved {result.saved_bytes} bytes "
            f"({result.original_size} -> {result.optimized_size})"
        )
    else:
        message = f"{result.source} unchanged ({result.original_size} bytes)"

    if verbose:
        message += f" [{result.strategy}]"
    return message


def main() -> None:
    args = parse_args()
    pngs = iter_pngs(args.paths, args.recursive)
    if not pngs:
        raise FileNotFoundError("No PNG files found for the provided paths")

    total_before = 0
    total_after = 0
    changed_count = 0

    for path in pngs:
        result = optimize_one(path, args)
        total_before += result.original_size
        total_after += result.optimized_size
        if result.changed:
            changed_count += 1
        print(format_result(result, args.verbose))

    saved = total_before - total_after
    print(
        f"Optimized {changed_count}/{len(pngs)} PNGs, "
        f"saved {saved} bytes total ({total_before} -> {total_after})."
    )


if __name__ == "__main__":
    main()
