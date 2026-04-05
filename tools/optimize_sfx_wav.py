#!/usr/bin/env python3
"""
Optimize WAV sound effects by converting them to smaller PCM WAV files.

This script keeps files as WAV and is aimed at short sound effects.
Typical usage is downsampling and/or forcing mono while preserving the format.

Examples:
  python3 tools/optimize_sfx_wav.py assets/audio/sound_effects --recursive
  python3 tools/optimize_sfx_wav.py assets/audio/sound_effects --recursive --sample-rate 22050 --mono
  python3 tools/optimize_sfx_wav.py assets/audio/sound_effects/attack.wav --verbose
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


WAV_SUFFIX = ".wav"


@dataclass(frozen=True)
class SfxOptimizationResult:
    source: Path
    output: Path
    original_size: int
    optimized_size: int
    original_sample_rate: int | None
    optimized_sample_rate: int | None
    original_channels: int | None
    optimized_channels: int | None
    changed: bool

    @property
    def saved_bytes(self) -> int:
        return self.original_size - self.optimized_size


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Optimize WAV sound effects with ffmpeg.")
    parser.add_argument("paths", nargs="+", help="WAV files or directories to process")
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=22050,
        help="Target output sample rate. Default: 22050",
    )
    parser.add_argument(
        "--mono",
        action="store_true",
        help="Force mono output",
    )
    parser.add_argument(
        "--sample-format",
        default="s16",
        choices=["u8", "s16", "s24", "s32"],
        help="PCM sample format for the output WAV. Default: s16",
    )
    parser.add_argument(
        "--recursive",
        action="store_true",
        help="Recurse into directories. Default: only direct child WAVs.",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Replace each source file when the optimized WAV is smaller",
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
        help="Print sample-rate and channel details for each file",
    )
    return parser.parse_args()


def require_binary(name: str) -> None:
    if shutil.which(name) is None:
        raise FileNotFoundError(f"Required binary not found in PATH: {name}")


def iter_wavs(paths: Iterable[str], recursive: bool) -> list[Path]:
    found: list[Path] = []
    for raw in paths:
        path = Path(raw)
        if path.is_file():
            if path.suffix.lower() == WAV_SUFFIX:
                found.append(path)
            continue

        if not path.is_dir():
            raise FileNotFoundError(f"Path does not exist: {path}")

        pattern = "**/*.wav" if recursive else "*.wav"
        found.extend(sorted(child for child in path.glob(pattern) if child.is_file()))

    unique: dict[Path, None] = {}
    for path in found:
        unique[path] = None
    return list(unique.keys())


def probe_audio(path: Path) -> tuple[int | None, int | None]:
    command = [
        "ffprobe",
        "-v",
        "error",
        "-select_streams",
        "a:0",
        "-show_entries",
        "stream=sample_rate,channels",
        "-of",
        "json",
        str(path),
    ]
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    payload = json.loads(completed.stdout or "{}")
    streams = payload.get("streams", [])
    if not streams:
        return None, None
    stream = streams[0]
    sample_rate = stream.get("sample_rate")
    channels = stream.get("channels")
    return (int(sample_rate) if sample_rate is not None else None,
            int(channels) if channels is not None else None)


def build_output_path(source: Path, args: argparse.Namespace) -> Path:
    if args.in_place:
        return source
    if args.output_dir:
        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        return output_dir / source.name
    return source.with_name(f"{source.stem}{args.suffix}{source.suffix}")


def pcm_codec(sample_format: str) -> str:
    return {
        "u8": "pcm_u8",
        "s16": "pcm_s16le",
        "s24": "pcm_s24le",
        "s32": "pcm_s32le",
    }[sample_format]


def reencode_wav(
    source: Path,
    destination: Path,
    sample_rate: int,
    mono: bool,
    sample_format: str,
) -> None:
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
        pcm_codec(sample_format),
        "-ar",
        str(sample_rate),
    ]
    if mono:
        command.extend(["-ac", "1"])
    command.append(str(destination))
    subprocess.run(command, check=True)


def optimize_one(source: Path, args: argparse.Namespace) -> SfxOptimizationResult:
    original_size = source.stat().st_size
    original_sample_rate, original_channels = probe_audio(source)
    output_path = build_output_path(source, args)

    with tempfile.TemporaryDirectory(prefix="cipherbound-sfx-opt-") as temp_dir:
        temp_output = Path(temp_dir) / source.name
        reencode_wav(
            source,
            temp_output,
            sample_rate=args.sample_rate,
            mono=args.mono,
            sample_format=args.sample_format,
        )

        optimized_size = temp_output.stat().st_size
        optimized_sample_rate, optimized_channels = probe_audio(temp_output)
        changed = optimized_size < original_size

        if changed or args.keep_larger:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(temp_output, output_path)

    return SfxOptimizationResult(
        source=source,
        output=output_path,
        original_size=original_size,
        optimized_size=optimized_size,
        original_sample_rate=original_sample_rate,
        optimized_sample_rate=optimized_sample_rate,
        original_channels=original_channels,
        optimized_channels=optimized_channels,
        changed=changed,
    )


def format_result(result: SfxOptimizationResult, verbose: bool) -> str:
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
            f" [{result.original_sample_rate}Hz/{result.original_channels}ch -> "
            f"{result.optimized_sample_rate}Hz/{result.optimized_channels}ch]"
        )
    return message


def main() -> None:
    args = parse_args()
    require_binary("ffmpeg")
    require_binary("ffprobe")

    wavs = iter_wavs(args.paths, args.recursive)
    if not wavs:
        raise FileNotFoundError("No WAV files found for the provided paths")

    total_before = 0
    total_after = 0
    changed_count = 0

    for source in wavs:
        result = optimize_one(source, args)
        total_before += result.original_size
        total_after += result.optimized_size
        if result.changed:
            changed_count += 1
        print(format_result(result, args.verbose))

    saved = total_before - total_after
    print(
        f"Optimized {changed_count}/{len(wavs)} WAVs, "
        f"saved {saved} bytes total ({total_before} -> {total_after})."
    )


if __name__ == "__main__":
    main()
