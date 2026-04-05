#!/usr/bin/env python3
"""
Optimize music assets by re-encoding MP3 files to a lower target bitrate.

This script is intentionally conservative:
- it only touches `.mp3` files
- it does not trim silence
- it does not change format
- it only writes the optimized result when the new file is smaller

Examples:
  python3 tools/optimize_audio.py assets/audio --recursive
  python3 tools/optimize_audio.py assets/audio --recursive --bitrate 128k --in-place
  python3 tools/optimize_audio.py assets/audio/022\\ Violet\\ City.mp3 --verbose
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


MP3_SUFFIX = ".mp3"


@dataclass(frozen=True)
class AudioOptimizationResult:
    source: Path
    output: Path
    original_size: int
    optimized_size: int
    original_bitrate: int | None
    optimized_bitrate: int | None
    changed: bool

    @property
    def saved_bytes(self) -> int:
        return self.original_size - self.optimized_size


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Optimize MP3 music assets with ffmpeg.")
    parser.add_argument("paths", nargs="+", help="MP3 files or directories to process")
    parser.add_argument(
        "--bitrate",
        default="128k",
        help="Target MP3 bitrate passed to ffmpeg, for example 128k or 160k",
    )
    parser.add_argument(
        "--sample-rate",
        type=int,
        help="Optional output sample rate, for example 44100 or 32000",
    )
    parser.add_argument(
        "--recursive",
        action="store_true",
        help="Recurse into directories. Default: only direct child MP3s.",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Replace each source file when the optimized MP3 is smaller",
    )
    parser.add_argument(
        "--output-dir",
        help="Write optimized files to this directory instead of next to the source file",
    )
    parser.add_argument(
        "--suffix",
        default="_optimized",
        help="Suffix for output files when not using --in-place or --output-dir",
    )
    parser.add_argument(
        "--keep-larger",
        action="store_true",
        help="Write the re-encoded file even if it is not smaller",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print bitrate details for each processed file",
    )
    return parser.parse_args()


def require_binary(name: str) -> None:
    if shutil.which(name) is None:
        raise FileNotFoundError(f"Required binary not found in PATH: {name}")


def iter_mp3s(paths: Iterable[str], recursive: bool) -> list[Path]:
    found: list[Path] = []
    for raw in paths:
        path = Path(raw)
        if path.is_file():
            if path.suffix.lower() == MP3_SUFFIX:
                found.append(path)
            continue

        if not path.is_dir():
            raise FileNotFoundError(f"Path does not exist: {path}")

        pattern = "**/*.mp3" if recursive else "*.mp3"
        found.extend(sorted(child for child in path.glob(pattern) if child.is_file()))

    unique: dict[Path, None] = {}
    for path in found:
        unique[path] = None
    return list(unique.keys())


def probe_bitrate(path: Path) -> int | None:
    command = [
        "ffprobe",
        "-v",
        "error",
        "-select_streams",
        "a:0",
        "-show_entries",
        "stream=bit_rate",
        "-of",
        "json",
        str(path),
    ]
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    payload = json.loads(completed.stdout or "{}")
    streams = payload.get("streams", [])
    if not streams:
        return None
    raw_bitrate = streams[0].get("bit_rate")
    if raw_bitrate is None:
        return None
    return int(raw_bitrate)


def build_output_path(source: Path, args: argparse.Namespace) -> Path:
    if args.in_place:
        return source
    if args.output_dir:
        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        return output_dir / source.name
    return source.with_name(f"{source.stem}{args.suffix}{source.suffix}")


def reencode_mp3(source: Path, destination: Path, bitrate: str, sample_rate: int | None) -> None:
    command = [
        "ffmpeg",
        "-y",
        "-v",
        "error",
        "-i",
        str(source),
        "-map_metadata",
        "-1",
        "-vn",
        "-c:a",
        "libmp3lame",
        "-b:a",
        bitrate,
    ]
    if sample_rate is not None:
        command.extend(["-ar", str(sample_rate)])
    command.append(str(destination))
    subprocess.run(command, check=True)


def optimize_one(source: Path, args: argparse.Namespace) -> AudioOptimizationResult:
    original_size = source.stat().st_size
    original_bitrate = probe_bitrate(source)
    output_path = build_output_path(source, args)

    with tempfile.TemporaryDirectory(prefix="cipherbound-audio-opt-") as temp_dir:
        temp_output = Path(temp_dir) / source.name
        reencode_mp3(source, temp_output, args.bitrate, args.sample_rate)

        optimized_size = temp_output.stat().st_size
        optimized_bitrate = probe_bitrate(temp_output)
        changed = optimized_size < original_size

        if changed or args.keep_larger:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(temp_output, output_path)

    return AudioOptimizationResult(
        source=source,
        output=output_path,
        original_size=original_size,
        optimized_size=optimized_size,
        original_bitrate=original_bitrate,
        optimized_bitrate=optimized_bitrate,
        changed=changed,
    )


def format_bitrate(value: int | None) -> str:
    if value is None:
        return "unknown"
    return f"{value // 1000}k"


def format_result(result: AudioOptimizationResult, verbose: bool) -> str:
    if result.changed:
        message = (
            f"{result.source} -> {result.output} "
            f"saved {result.saved_bytes} bytes "
            f"({result.original_size} -> {result.optimized_size})"
        )
    else:
        message = f"{result.source} unchanged ({result.original_size} bytes)"

    if verbose:
        message += (
            f" [{format_bitrate(result.original_bitrate)} -> "
            f"{format_bitrate(result.optimized_bitrate)}]"
        )
    return message


def main() -> None:
    args = parse_args()
    require_binary("ffmpeg")
    require_binary("ffprobe")

    mp3s = iter_mp3s(args.paths, args.recursive)
    if not mp3s:
        raise FileNotFoundError("No MP3 files found for the provided paths")

    total_before = 0
    total_after = 0
    changed_count = 0

    for source in mp3s:
        result = optimize_one(source, args)
        total_before += result.original_size
        total_after += result.optimized_size
        if result.changed:
            changed_count += 1
        print(format_result(result, args.verbose))

    saved = total_before - total_after
    print(
        f"Optimized {changed_count}/{len(mp3s)} MP3s, "
        f"saved {saved} bytes total ({total_before} -> {total_after})."
    )


if __name__ == "__main__":
    main()
