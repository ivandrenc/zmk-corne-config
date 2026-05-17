#!/usr/bin/env python3
"""
nicview_frames.py — Convert pixel art frames to LVGL C source for the
ZMK nice!view Sharp LS0XX 160×68 display.

Single-animation usage
----------------------
    python3 tools/nicview_frames.py <frames_dir> [options]

    Examples:
        # Write to the shield asset file with a back-and-forth sequence
        python3 tools/nicview_frames.py "Claude Looking JPG Sequence/" \\
            --output boards/shields/claude_view/assets/claude_art.c \\
            --prefix look \\
            --sequence 1,2,3,4,3,2 \\
            --ms-per-frame 60

Multi-animation manifest usage
-------------------------------
    python3 tools/nicview_frames.py --manifest tools/animations.toml \\
        --output boards/shields/claude_view/assets/claude_art.c

    The manifest is a TOML file with an [[animations]] array:

        [[animations]]
        name       = "look"
        dir        = "Claude Looking JPG Sequence"
        ping_pong  = true
        ms_per_frame = 60

        [[animations]]
        name       = "gym"
        dir        = "claude-gym-animation-frames"
        ms_per_frame = 80

    Multi-animation mode emits a `struct claude_animation animations[]` table
    and a `uint8_t animation_count` constant. The firmware can switch between
    them at runtime via behaviors in the keymap (see config/eyelash_corne.keymap).

Pipeline (per frame)
--------------------
    1. Collect all image files in <frames_dir>, sorted alphabetically.
    2. Detect the bounding box of non-background content in each frame
       using ImageMagick's -trim. Compute the union across all frames.
    3. Add padding around the detected crop (--padding, default 20 px).
    4. Resize to FIT within 68×140 portrait, preserving aspect ratio.
    5. White-letterbox to exactly 68×140.
    6. Grayscale + Bayer ordered dithering (o4x4).
    7. Rotate 90° CW → stored as 140×68 landscape bitmap.
       (Compensates for the nice!view panel's physical portrait mounting.)
    8. Pack as LVGL CF_INDEXED_1BIT (index 0=black, index 1=white).
    9. Write named lv_img_dsc_t structs to the output .c file.

The --no-rotate flag skips step 7 if your display does not need it.
"""

import argparse
import math
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DISPLAY_W = 140     # stored image width  (LVGL landscape)
DISPLAY_H = 68      # stored image height
PORTRAIT_W = 68     # intermediate portrait width  (before 90° CW rotation)
PORTRAIT_H = 140

BYTES_PER_ROW = math.ceil(DISPLAY_W / 8)           # 18
DATA_SIZE = 8 + BYTES_PER_ROW * DISPLAY_H          # 8 palette + 18×68 = 1232

SUPPORTED_EXTENSIONS = {".jpg", ".jpeg", ".png", ".bmp", ".gif", ".webp"}


# ---------------------------------------------------------------------------
# ImageMagick helpers
# ---------------------------------------------------------------------------

def magick(*args: str) -> subprocess.CompletedProcess:
    cmd = ["magick", *args]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[magick error] {' '.join(cmd)}", file=sys.stderr)
        print(result.stderr.strip(), file=sys.stderr)
        sys.exit(1)
    return result


def check_magick() -> None:
    result = subprocess.run(["magick", "--version"], capture_output=True)
    if result.returncode != 0:
        print("ERROR: ImageMagick not found. Install with: brew install imagemagick",
              file=sys.stderr)
        sys.exit(1)


# ---------------------------------------------------------------------------
# Crop detection
# ---------------------------------------------------------------------------

def detect_bbox(path: str, fuzz: int) -> tuple[int, int, int, int] | None:
    result = magick(path, "-fuzz", f"{fuzz}%", "-trim",
                    "-format", "%wx%h%O\n", "info:")
    m = re.match(r"(\d+)x(\d+)\+(\d+)\+(\d+)", result.stdout.strip())
    if not m:
        return None
    w, h, x, y = int(m[1]), int(m[2]), int(m[3]), int(m[4])
    return x, y, x + w, y + h


def union_crop(frames: list[str], fuzz: int,
               padding: int) -> tuple[int, int, int, int]:
    """Compute padded union bounding box across all frames."""
    print(f"  Detecting crop (fuzz={fuzz}%, padding={padding}px)...", file=sys.stderr)
    x1_all, y1_all, x2_all, y2_all = [], [], [], []
    for f in frames:
        bbox = detect_bbox(f, fuzz)
        if bbox:
            x1_all.append(bbox[0])
            y1_all.append(bbox[1])
            x2_all.append(bbox[2])
            y2_all.append(bbox[3])
            print(f"    {Path(f).name}: {bbox[2]-bbox[0]}×{bbox[3]-bbox[1]}"
                  f" at +{bbox[0]}+{bbox[1]}", file=sys.stderr)
        else:
            print(f"    {Path(f).name}: trim failed, skipping", file=sys.stderr)

    if not x1_all:
        print("ERROR: Could not detect content in any frame.", file=sys.stderr)
        sys.exit(1)

    x1 = max(0, min(x1_all) - padding)
    y1 = max(0, min(y1_all) - padding)
    x2 = max(x2_all) + padding
    y2 = max(y2_all) + padding
    print(f"  Union crop: {x2-x1}×{y2-y1} at +{x1}+{y1}", file=sys.stderr)
    return x1, y1, x2, y2


# ---------------------------------------------------------------------------
# Frame conversion
# ---------------------------------------------------------------------------

def convert_frame(src: str, crop: tuple[int, int, int, int],
                  rotate: bool, dither: str, threshold: int | None,
                  tmp_dir: str) -> str:
    """Process one source image → 1-bit BMP. Returns BMP path."""
    x1, y1, x2, y2 = crop
    crop_arg = f"{x2-x1}x{y2-y1}+{x1}+{y1}"
    out_bmp = os.path.join(tmp_dir, Path(src).stem + ".bmp")

    cmd = [src,
           "-crop", crop_arg, "+repage",
           "-resize", f"{PORTRAIT_W}x{PORTRAIT_H}",
           "-background", "white", "-gravity", "Center",
           "-extent", f"{PORTRAIT_W}x{PORTRAIT_H}",
           "-colorspace", "Gray"]

    if threshold is not None:
        cmd += ["-threshold", f"{threshold}%"]
    else:
        cmd += ["-ordered-dither", dither]

    cmd += ["-type", "Bilevel"]

    if rotate:
        cmd += ["-rotate", "90"]

    cmd.append(out_bmp)
    magick(*cmd)
    return out_bmp


# ---------------------------------------------------------------------------
# Pixel extraction & packing
# ---------------------------------------------------------------------------

def bmp_to_pixels(bmp_path: str) -> list[int]:
    """Return list of 0/1 pixel values for a 1-bit BMP."""
    result = magick(bmp_path, "txt:-")
    pixels = []
    for line in result.stdout.splitlines():
        if line.startswith("#"):
            continue
        m = re.search(r"\((\d+(?:\.\d+)?),", line)
        if m:
            pixels.append(0 if float(m[1]) < 128 else 1)
    return pixels


def pack_row(row_pixels: list[int]) -> list[int]:
    padded = row_pixels + [0] * (BYTES_PER_ROW * 8 - DISPLAY_W)
    return [
        sum((padded[i * 8 + b] << (7 - b)) for b in range(8))
        for i in range(BYTES_PER_ROW)
    ]


def pixels_to_lvgl_bytes(pixels: list[int]) -> list[int]:
    data = [0x00, 0x00, 0x00, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF]
    for row in range(DISPLAY_H):
        data.extend(pack_row(pixels[row * DISPLAY_W:(row + 1) * DISPLAY_W]))
    return data


# ---------------------------------------------------------------------------
# C code generation helpers
# ---------------------------------------------------------------------------

def c_frame_block(name: str, data: list[int]) -> str:
    """Render one lv_img_dsc_t block (array + descriptor)."""
    lines = [
        f"static const LV_ATTRIBUTE_MEM_ALIGN uint8_t {name}_map[] = {{",
        "    0x00, 0x00, 0x00, 0xff,  /* index 0 = black */",
        "    0xff, 0xff, 0xff, 0xff,  /* index 1 = white */",
    ]
    hex_vals = [f"0x{b:02x}" for b in data[8:]]
    chunk = 16
    for i in range(0, len(hex_vals), chunk):
        lines.append("    " + ", ".join(hex_vals[i:i + chunk]) + ",")
    lines += [
        "};",
        f"const lv_img_dsc_t {name} = {{",
        f"    .header.cf          = LV_IMG_CF_INDEXED_1BIT,",
        f"    .header.always_zero = 0,",
        f"    .header.reserved    = 0,",
        f"    .header.w           = {DISPLAY_W},",
        f"    .header.h           = {DISPLAY_H},",
        f"    .data_size          = {DATA_SIZE},",
        f"    .data               = {name}_map,",
        "};",
    ]
    return "\n".join(lines)


def pipeline_str(dither: str, threshold: int | None, rotate: bool) -> str:
    dither_part = (f"threshold {threshold}%" if threshold is not None
                   else f"ordered dither {dither}")
    rot_part = " -> rotate 90 CW" if rotate else ""
    return (f"auto-detect crop -> fit {PORTRAIT_W}x{PORTRAIT_H} portrait "
            f"-> white letterbox -> grayscale -> {dither_part}"
            f"{rot_part} -> {DISPLAY_W}x{DISPLAY_H} stored")


# ---------------------------------------------------------------------------
# Single-animation C file
# ---------------------------------------------------------------------------

def build_single_c(unique_names: list[str], unique_bmps: list[str],
                   unique_indices: list[int], sequence: list[int], frames_dir: str,
                   ms_per_frame: int, dither: str,
                   threshold: int | None, rotate: bool) -> str:
    seq_str = ",".join(str(i) for i in sequence)
    parts = [
        "/*",
        " * Generated by tools/nicview_frames.py",
        f" * Source:   {frames_dir}",
        f" * Unique:   {len(unique_names)} frame(s)   Sequence: {seq_str} ({len(sequence)} entries)",
        f" * ms/frame: {ms_per_frame}",
        f" * Pipeline: {pipeline_str(dither, threshold, rotate)}",
        " */",
        '#include "claude_art.h"',
        "",
    ]

    for name, bmp in zip(unique_names, unique_bmps):
        print(f"  Packing {Path(bmp).name} → {name} ...", file=sys.stderr)
        pixels = bmp_to_pixels(bmp)
        if len(pixels) != DISPLAY_W * DISPLAY_H:
            print(f"    WARNING: expected {DISPLAY_W * DISPLAY_H} pixels, "
                  f"got {len(pixels)} — padding.", file=sys.stderr)
            pixels = (pixels + [0] * DISPLAY_W * DISPLAY_H)[:DISPLAY_W * DISPLAY_H]
        parts.append(c_frame_block(name, pixels_to_lvgl_bytes(pixels)))
        parts.append("")

    by_idx = dict(zip(unique_indices, unique_names))
    seq_names = [f"&{by_idx[i]}" for i in sequence]
    parts += [
        "const lv_img_dsc_t * const claude_frames[] = {",
        *[f"    {n}," for n in seq_names],
        "};",
        f"const uint16_t claude_frame_count = {len(sequence)};",
        f"const uint16_t claude_frame_ms    = {ms_per_frame};",
        "",
        "/* Single-animation shim — exposes the animations[] table expected by",
        " * custom_status_screen.c so single- and multi-animation builds share",
        " * the same firmware code. */",
        "const struct claude_animation animations[] = {{",
        f"    .name        = \"{Path(frames_dir).name}\",",
        f"    .frames      = claude_frames,",
        f"    .count       = {len(sequence)},",
        f"    .ms_per_frame = {ms_per_frame},",
        "}};",
        "const uint8_t animation_count = 1;",
        "",
    ]
    return "\n".join(parts)


# ---------------------------------------------------------------------------
# Multi-animation C file (manifest mode)
# ---------------------------------------------------------------------------

class AnimSpec:
    def __init__(self, name: str, frames_dir: str, sequence: list[int] | None,
                 ms_per_frame: int, fuzz: int, padding: int,
                 dither: str, threshold: int | None, rotate: bool):
        self.name = name
        self.frames_dir = frames_dir
        self.sequence = sequence          # None → all frames in order
        self.ms_per_frame = ms_per_frame
        self.fuzz = fuzz
        self.padding = padding
        self.dither = dither
        self.threshold = threshold
        self.rotate = rotate


def process_anim(spec: AnimSpec, tmp_dir: str
                 ) -> tuple[list[str], list[str], list[int]]:
    """Convert one AnimSpec. Returns (unique_names, unique_bmps, resolved_sequence)."""
    frames = collect_frames(spec.frames_dir)
    verify_frame_numbering(frames, spec.name)
    n = len(frames)
    print(f"\n[{spec.name}] {n} frame(s) in '{spec.frames_dir}'", file=sys.stderr)

    sequence = (spec.sequence if spec.sequence is not None
                else list(range(1, n + 1)))

    seen: dict[int, str] = {}
    for idx in sequence:
        if idx not in seen:
            seen[idx] = frames[idx - 1]
    unique_indices = list(seen.keys())
    unique_sources = [seen[i] for i in unique_indices]
    unique_names = [f"{spec.name}_{i}" for i in unique_indices]

    print(f"  Unique: {unique_indices}  Sequence: {sequence}", file=sys.stderr)

    crop = union_crop(unique_sources, spec.fuzz, spec.padding)

    print(f"  Converting {len(unique_sources)} frame(s)...", file=sys.stderr)
    unique_bmps = []
    for src, cname in zip(unique_sources, unique_names):
        print(f"    {Path(src).name} → {cname}", file=sys.stderr)
        bmp = convert_frame(src, crop, spec.rotate, spec.dither,
                            spec.threshold, tmp_dir)
        unique_bmps.append(bmp)

    return unique_names, unique_bmps, unique_indices, sequence


def build_multi_c(specs: list[AnimSpec], tmp_dir: str) -> str:
    parts = [
        "/*",
        " * Generated by tools/nicview_frames.py --manifest",
        " * Contains " + str(len(specs)) + " animation(s):",
    ]
    for i, s in enumerate(specs):
        parts.append(f" *   [{i}] {s.name}  ({s.frames_dir})")
    parts += [
        " */",
        '#include "claude_art.h"',
        "",
    ]

    anim_results: list[tuple[str, list[str], list[int], int]] = []

    for spec in specs:
        unique_names, unique_bmps, unique_indices, sequence = process_anim(spec, tmp_dir)
        print(f"\n  Packing [{spec.name}]...", file=sys.stderr)
        for name, bmp in zip(unique_names, unique_bmps):
            print(f"    {Path(bmp).name} → {name} ...", file=sys.stderr)
            pixels = bmp_to_pixels(bmp)
            if len(pixels) != DISPLAY_W * DISPLAY_H:
                print(f"      WARNING: {len(pixels)} pixels, expected "
                      f"{DISPLAY_W * DISPLAY_H} — padding.", file=sys.stderr)
                pixels = (pixels + [0] * DISPLAY_W * DISPLAY_H)[:DISPLAY_W * DISPLAY_H]
            parts.append(c_frame_block(name, pixels_to_lvgl_bytes(pixels)))
            parts.append("")
        anim_results.append((spec.name, unique_names, unique_indices, sequence, spec.ms_per_frame))

    # Per-animation sequence arrays
    for anim_name, unique_names, unique_indices, sequence, ms in anim_results:
        by_idx = dict(zip(unique_indices, unique_names))
        seq_entries = [f"&{by_idx[i]}" for i in sequence]
        arr_name = f"{anim_name}_frames"
        parts += [
            f"static const lv_img_dsc_t * const {arr_name}[] = {{",
            *[f"    {e}," for e in seq_entries],
            "};",
            "",
        ]

    # Global animations table
    parts += [
        "const struct claude_animation animations[] = {",
    ]
    for anim_name, _, _unique_indices, sequence, ms in anim_results:
        parts += [
            "    {",
            f'        .name        = "{anim_name}",',
            f"        .frames      = {anim_name}_frames,",
            f"        .count       = {len(sequence)},",
            f"        .ms_per_frame = {ms},",
            "    },",
        ]
    parts += [
        "};",
        f"const uint8_t animation_count = {len(specs)};",
        "",
        "/* Legacy single-anim aliases (first animation) */",
        "const lv_img_dsc_t * const * const claude_frames  = animations[0].frames;",
        "const uint16_t claude_frame_count                 = animations[0].count;",
        "const uint16_t claude_frame_ms                    = animations[0].ms_per_frame;",
        "",
    ]
    return "\n".join(parts)


# ---------------------------------------------------------------------------
# Manifest parsing (TOML)
# ---------------------------------------------------------------------------

def ping_pong_sequence(n_frames: int) -> list[int]:
    """1..N then N-1..2 (endpoints not repeated on the way back)."""
    if n_frames <= 0:
        return []
    if n_frames == 1:
        return [1]
    return list(range(1, n_frames + 1)) + list(range(n_frames - 1, 1, -1))


def sequence_skip_range(n_frames: int, skip_from: int, skip_to: int) -> list[int]:
    """Play 1..skip_from-1 and skip_to+1..N (1-based, inclusive skip range)."""
    if skip_from < 1 or skip_to > n_frames or skip_from > skip_to:
        print(f"ERROR: skip_range [{skip_from}, {skip_to}] invalid for {n_frames} frame(s).",
              file=sys.stderr)
        sys.exit(1)
    return list(range(1, skip_from)) + list(range(skip_to + 1, n_frames + 1))


def load_manifest(path: str, defaults: dict) -> list[AnimSpec]:
    """Parse animations.toml → list[AnimSpec]."""
    try:
        import tomllib  # Python 3.11+
    except ImportError:
        try:
            import tomli as tomllib  # pip install tomli
        except ImportError:
            print("ERROR: tomllib not available. Use Python 3.11+ or: pip install tomli",
                  file=sys.stderr)
            sys.exit(1)

    with open(path, "rb") as f:
        doc = tomllib.load(f)

    specs = []
    for entry in doc.get("animations", []):
        name = entry["name"]
        frames_dir = entry["dir"]
        seq_str = entry.get("sequence", None)
        ping_pong = entry.get("ping_pong", False)
        skip_range = entry.get("skip_range", None)
        n_frames = len(collect_frames(frames_dir))
        if seq_str:
            if ping_pong or skip_range:
                print(f"WARNING: [{name}] has sequence; ignoring ping_pong/skip_range.",
                      file=sys.stderr)
            sequence = parse_sequence(seq_str, n_frames)
        elif skip_range is not None:
            if ping_pong:
                print(f"WARNING: [{name}] has both skip_range and ping_pong; using skip_range.",
                      file=sys.stderr)
            if not isinstance(skip_range, list) or len(skip_range) != 2:
                print(f"ERROR: [{name}] skip_range must be [from, to] (1-based inclusive).",
                      file=sys.stderr)
                sys.exit(1)
            sequence = sequence_skip_range(n_frames, int(skip_range[0]), int(skip_range[1]))
        elif ping_pong:
            sequence = ping_pong_sequence(n_frames)
        else:
            sequence = None
        specs.append(AnimSpec(
            name=name,
            frames_dir=frames_dir,
            sequence=sequence,
            ms_per_frame=entry.get("ms_per_frame", defaults["ms_per_frame"]),
            fuzz=entry.get("fuzz", defaults["fuzz"]),
            padding=entry.get("padding", defaults["padding"]),
            dither=entry.get("dither", defaults["dither"]),
            threshold=entry.get("threshold", defaults["threshold"]),
            rotate=not entry.get("no_rotate", not defaults["rotate"]),
        ))
    if not specs:
        print("ERROR: manifest contains no [[animations]] entries.", file=sys.stderr)
        sys.exit(1)
    return specs


# ---------------------------------------------------------------------------
# CLI helpers (shared)
# ---------------------------------------------------------------------------

def collect_frames(frames_dir: str) -> list[str]:
    p = Path(frames_dir)
    if not p.is_dir():
        print(f"ERROR: '{frames_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)
    files = sorted(
        str(f) for f in p.iterdir()
        if f.suffix.lower() in SUPPORTED_EXTENSIONS and not f.name.startswith(".")
    )
    if not files:
        print(f"ERROR: No image files found in '{frames_dir}'.", file=sys.stderr)
        sys.exit(1)
    return files


def verify_frame_numbering(frames: list[str], anim_name: str) -> None:
    """Warn if sorted filenames are not contiguous 1..N (playback order depends on this)."""
    nums: list[int] = []
    for path in frames:
        m = re.search(r"(\d+)", Path(path).name)
        if not m:
            print(f"WARNING: [{anim_name}] no frame number in '{Path(path).name}'", file=sys.stderr)
            return
        nums.append(int(m.group(1)))
    expected = list(range(1, len(frames) + 1))
    if nums != expected:
        print(f"WARNING: [{anim_name}] frame numbers are not 1..{len(frames)} after sort.",
              file=sys.stderr)
        print(f"  First mismatch: expected {expected[len(nums)-1] if len(nums) < len(expected) else '?'}, "
              f"got {nums}", file=sys.stderr)


def parse_sequence(seq_str: str, n_frames: int) -> list[int]:
    try:
        seq = [int(x.strip()) for x in seq_str.split(",")]
    except ValueError:
        print(f"ERROR: sequence must be comma-separated integers, got '{seq_str}'",
              file=sys.stderr)
        sys.exit(1)
    for idx in seq:
        if idx < 1 or idx > n_frames:
            print(f"ERROR: sequence index {idx} out of range 1..{n_frames}",
                  file=sys.stderr)
            sys.exit(1)
    return seq


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert pixel art frames to LVGL C source for the ZMK nice!view display.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    # Mode
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("frames_dir", nargs="?", default=None,
                      help="Directory of frame images (single-animation mode).")
    mode.add_argument("--manifest", "-m", default=None, metavar="FILE",
                      help="TOML manifest for multi-animation mode.")

    parser.add_argument("-o", "--output", default="-",
                        help="Output .c file path. '-' for stdout (default).")
    parser.add_argument("-p", "--prefix", default="claude_frame",
                        help="C variable name prefix (single-animation mode, default: claude_frame).")
    parser.add_argument("-s", "--sequence", default=None,
                        help="1-based frame indices for animation loop, e.g. '1,2,3,4,3,2'.")
    parser.add_argument("--ms-per-frame", type=int, default=200,
                        help="Milliseconds per frame (default: 200).")
    parser.add_argument("--fuzz", type=int, default=10,
                        help="ImageMagick background fuzz %% for crop detection (default: 10).")
    parser.add_argument("--padding", type=int, default=20,
                        help="Padding around auto-detected crop in px (default: 20).")
    parser.add_argument("--dither", default="o4x4",
                        help="Ordered dither pattern (default: o4x4).")
    parser.add_argument("--threshold", type=int, default=None,
                        help="Hard threshold %% instead of dithering.")
    parser.add_argument("--no-rotate", action="store_true",
                        help="Skip the 90° CW pre-rotation step.")

    args = parser.parse_args()

    if args.frames_dir is None and args.manifest is None:
        parser.error("Provide either a frames_dir or --manifest FILE.")

    check_magick()

    rotate = not args.no_rotate
    defaults = dict(ms_per_frame=args.ms_per_frame, fuzz=args.fuzz,
                    padding=args.padding, dither=args.dither,
                    threshold=args.threshold, rotate=rotate)

    with tempfile.TemporaryDirectory(prefix="nicview_") as tmp_dir:

        if args.manifest:
            # ── Multi-animation manifest mode ────────────────────────────
            specs = load_manifest(args.manifest, defaults)
            print(f"Manifest: {len(specs)} animation(s):", file=sys.stderr)
            for s in specs:
                print(f"  • {s.name}  ({s.frames_dir})", file=sys.stderr)
            c_content = build_multi_c(specs, tmp_dir)

        else:
            # ── Single-animation mode ─────────────────────────────────────
            frames = collect_frames(args.frames_dir)
            n = len(frames)
            print(f"Found {n} frame(s) in '{args.frames_dir}':", file=sys.stderr)
            for f in frames:
                print(f"  {Path(f).name}", file=sys.stderr)

            sequence = (parse_sequence(args.sequence, n) if args.sequence
                        else list(range(1, n + 1)))

            seen: dict[int, str] = {}
            for idx in sequence:
                if idx not in seen:
                    seen[idx] = frames[idx - 1]
            unique_indices = list(seen.keys())
            unique_sources = [seen[i] for i in unique_indices]
            unique_names = [f"{args.prefix}_{i}" for i in unique_indices]

            print(f"\nUnique: {unique_indices}  Sequence: {sequence}", file=sys.stderr)

            crop = union_crop(unique_sources, args.fuzz, args.padding)

            print(f"\nConverting frames...", file=sys.stderr)
            unique_bmps = []
            for src, name in zip(unique_sources, unique_names):
                print(f"  {Path(src).name} → {name}", file=sys.stderr)
                bmp = convert_frame(src, crop, rotate, args.dither,
                                    args.threshold, tmp_dir)
                unique_bmps.append(bmp)

            print(f"\nPacking pixel data...", file=sys.stderr)
            c_content = build_single_c(
                unique_names, unique_bmps, unique_indices, sequence, args.frames_dir,
                args.ms_per_frame, args.dither, args.threshold, rotate,
            )

    if args.output == "-":
        sys.stdout.write(c_content)
    else:
        out_path = Path(args.output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(c_content)
        print(f"\nWrote {out_path} ({out_path.stat().st_size} bytes, "
              f"{len(c_content.splitlines())} lines)", file=sys.stderr)

    print("\nDone.", file=sys.stderr)


if __name__ == "__main__":
    main()
