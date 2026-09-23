#!/usr/bin/env python3
"""Regenerate every Oblivion launcher icon from the high resolution loading symbol.

The launcher artwork is derived from the in-game loading symbol texture
(app/src/main/assets/textures/ui/loading_symbol.png) so the icons stay sharp at
every density instead of being upscaled from a 32x32 bitmap.

That texture is a Bethesda asset and is gitignored, so it is absent from a fresh
clone. A sha256 manifest (scripts/launcher_icon_manifest.json) is therefore
committed alongside the icons, letting `--check` verify the shipped artwork
anywhere while the pixel-exact comparison only runs where the texture exists.

Masked icons (the round legacy icon and both adaptive layers) are fitted by their
minimum enclosing circle rather than their bounding box. The glyph is asymmetric
-- it reaches furthest towards the upper left -- so bounding-box centring both
pushes it off-centre and understates how much room it needs.

Outputs (all under app/src/main/res):
  mipmap-<density>/ic_launcher.png              legacy square icon, background baked in
  mipmap-<density>/ic_launcher_round.png        legacy round icon, circular background
  mipmap-<density>/ic_launcher_foreground.png   adaptive icon foreground (transparent)
  mipmap-<density>/ic_launcher_monochrome.png   adaptive icon monochrome layer (white)

Run from the repository root:
    python scripts/generate_launcher_icons.py            # write the icons + manifest
    python scripts/generate_launcher_icons.py --check    # fail if the icons drifted
"""

import hashlib
import json
import random
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parent.parent
SOURCE = REPO_ROOT / "app/src/main/assets/textures/ui/loading_symbol.png"
RES_DIR = REPO_ROOT / "app/src/main/res"
RES_PREFIX = "app/src/main/res"
MANIFEST = REPO_ROOT / "scripts/launcher_icon_manifest.json"

BACKGROUND = (26, 26, 26, 255)  # #1A1A1A, matches drawable/ic_launcher_background.xml
ALPHA_THRESHOLD = 16
SUPERSAMPLE = 4

# density -> (legacy 48dp icon size, adaptive 108dp canvas size)
DENSITIES = {
    "ldpi": (36, 81),
    "mdpi": (48, 108),
    "hdpi": (72, 162),
    "xhdpi": (96, 216),
    "xxhdpi": (144, 324),
    "xxxhdpi": (192, 432),
}

# How much of the canvas the artwork occupies. The square legacy icon has no mask
# to survive, so it is sized by its bounding box; everything a circular or rounded
# mask can crop is sized by its enclosing circle instead.
LEGACY_FILL = 0.74    # bounding box longest side / canvas
ROUND_FILL = 0.62     # enclosing circle diameter / canvas
ADAPTIVE_FILL = 0.61  # enclosing circle diameter / canvas

# Android guarantees only the inner 66dp of the 108dp adaptive canvas stays
# visible, i.e. a circle of radius 33/108 = 30.6% of the canvas. ADAPTIVE_FILL is
# derived from that limit, so verify it rather than trusting the arithmetic.
SAFE_RADIUS = 33 / 108


def convex_hull(points):
    """Monotone chain hull of an (n, 2) float array."""
    unique = np.unique(points, axis=0)
    unique = unique[np.lexsort((unique[:, 1], unique[:, 0]))]

    def chain(sequence):
        out = []
        for point in sequence:
            while len(out) >= 2:
                (x1, y1), (x2, y2) = out[-2], out[-1]
                cross = (x2 - x1) * (point[1] - y1) - (y2 - y1) * (point[0] - x1)
                if cross > 0:
                    break
                out.pop()
            out.append((point[0], point[1]))
        return out

    lower = chain(unique)
    upper = chain(unique[::-1])
    return np.array(lower[:-1] + upper[:-1], dtype=float)


def _circle_from_two(first, second):
    centre = (first + second) / 2
    return centre, float(np.hypot(*(first - second)) / 2)


def _circle_from_three(first, second, third):
    ax, ay = first
    bx, by = second
    cx, cy = third
    determinant = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by))
    if abs(determinant) < 1e-12:
        return None
    ux = (
        (ax**2 + ay**2) * (by - cy)
        + (bx**2 + by**2) * (cy - ay)
        + (cx**2 + cy**2) * (ay - by)
    ) / determinant
    uy = (
        (ax**2 + ay**2) * (cx - bx)
        + (bx**2 + by**2) * (ax - cx)
        + (cx**2 + cy**2) * (bx - ax)
    ) / determinant
    centre = np.array([ux, uy])
    return centre, float(np.hypot(*(centre - first)))


def _bounding_circle(boundary):
    """Smallest circle through zero to three boundary points."""
    if not boundary:
        return None
    if len(boundary) == 1:
        return boundary[0], 0.0
    if len(boundary) == 2:
        return _circle_from_two(*boundary)
    best = None
    for i in range(3):
        for j in range(i + 1, 3):
            centre, radius = _circle_from_two(boundary[i], boundary[j])
            if all(np.hypot(*(p - centre)) <= radius + 1e-9 for p in boundary):
                best = (centre, radius)
    if best is None:
        best = _circle_from_three(*boundary)
    return best


def min_enclosing_circle(points):
    """Welzl's algorithm; only hull vertices can lie on the enclosing circle."""
    shuffled = list(convex_hull(points))
    random.Random(0).shuffle(shuffled)

    def solve(count, boundary):
        if count == 0 or len(boundary) == 3:
            return _bounding_circle(boundary)
        point = shuffled[count - 1]
        circle = solve(count - 1, boundary)
        if circle is not None and np.hypot(*(point - circle[0])) <= circle[1] + 1e-9:
            return circle
        return solve(count - 1, boundary + [point])

    return solve(len(shuffled), [])


def load_symbol():
    """Return the loading symbol cropped to its opaque bounds and its enclosing circle.

    The circle is (cx, cy, radius) in cropped-image pixels.
    """
    symbol = Image.open(SOURCE).convert("RGBA")
    alpha = np.array(symbol.getchannel("A"))
    ys, xs = np.nonzero(alpha > ALPHA_THRESHOLD)
    symbol = symbol.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))

    alpha = np.array(symbol.getchannel("A"))
    ys, xs = np.nonzero(alpha > ALPHA_THRESHOLD)
    points = np.column_stack([xs, ys]).astype(float)
    centre, radius = min_enclosing_circle(points)

    # The mask fits are only safe if the circle really does contain the artwork.
    reached = np.sqrt(((points - centre) ** 2).sum(axis=1)).max()
    if reached > radius + 1e-6:
        raise RuntimeError(f"enclosing circle {radius:.3f} excludes pixels at {reached:.3f}")
    return symbol, (centre[0], centre[1], radius)


def place(source, canvas_px, fill, enclosing=None):
    """Scale `source` to `fill` of the canvas and centre it on a transparent canvas.

    With `enclosing` the artwork is centred on that circle and `fill` is the circle's
    diameter, so no masked corner can be cropped. Without it the artwork is centred
    on its bounding box and `fill` is the bounding box's longest side.
    """
    width, height = source.size
    if enclosing is None:
        scale = canvas_px * fill / max(width, height)
        offset = (canvas_px / 2, canvas_px / 2)
    else:
        cx, cy, radius = enclosing
        scale = canvas_px * fill / (2 * radius)
        offset = (canvas_px / 2 - cx * scale, canvas_px / 2 - cy * scale)

    size = (max(1, round(width * scale)), max(1, round(height * scale)))
    art = source.resize(size, Image.LANCZOS)
    canvas = Image.new("RGBA", (canvas_px, canvas_px), (0, 0, 0, 0))
    canvas.paste(art, (round(offset[0]), round(offset[1])), art)
    return canvas


def compose(canvas_px, fill, source, enclosing=None, background=None, circle=False):
    """Render one icon at `canvas_px` using supersampling for clean edges."""
    big = canvas_px * SUPERSAMPLE
    canvas = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    if background is not None:
        draw = ImageDraw.Draw(canvas)
        if circle:
            draw.ellipse((0, 0, big - 1, big - 1), fill=background)
        else:
            draw.rectangle((0, 0, big - 1, big - 1), fill=background)
    canvas.alpha_composite(place(source, big, fill, enclosing))
    return canvas.resize((canvas_px, canvas_px), Image.LANCZOS)


def monochrome(canvas_px, fill, source, enclosing):
    """Solid white silhouette; Android tints this layer for themed icons."""
    art = place(source, canvas_px * SUPERSAMPLE, fill, enclosing)
    white = Image.new("L", art.size, 255)
    layer = Image.merge("RGBA", (white, white, white, art.getchannel("A")))
    return layer.resize((canvas_px, canvas_px), Image.LANCZOS)


def build_icons(symbol, enclosing):
    """Return {path relative to res/: image} for every icon this script owns."""
    icons = {}
    for density, (legacy_px, adaptive_px) in DENSITIES.items():
        folder = f"mipmap-{density}"
        icons[f"{folder}/ic_launcher.png"] = compose(
            legacy_px, LEGACY_FILL, symbol, background=BACKGROUND
        )
        icons[f"{folder}/ic_launcher_round.png"] = compose(
            legacy_px, ROUND_FILL, symbol, enclosing, BACKGROUND, circle=True
        )
        icons[f"{folder}/ic_launcher_foreground.png"] = compose(
            adaptive_px, ADAPTIVE_FILL, symbol, enclosing
        )
        icons[f"{folder}/ic_launcher_monochrome.png"] = monochrome(
            adaptive_px, ADAPTIVE_FILL, symbol, enclosing
        )
    return icons


def write_manifest(icons):
    """Record the sha256 of each icon so `--check` works without the texture."""
    manifest = {
        f"{RES_PREFIX}/{rel}": hashlib.sha256((RES_DIR / rel).read_bytes()).hexdigest()
        for rel in icons
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def check_manifest():
    """Return (drift descriptions, res-relative paths that already drifted)."""
    expected = json.loads(MANIFEST.read_text(encoding="utf-8"))
    problems = []
    drifted = set()
    for rel, digest in sorted(expected.items()):
        path = REPO_ROOT / rel
        key = rel[len(RES_PREFIX) + 1 :]
        if not path.exists():
            problems.append(f"{rel} is missing")
        elif hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            problems.append(f"{rel} no longer matches the committed icon")
        else:
            continue
        drifted.add(key)
    return problems, drifted


def check_pixels(symbol, enclosing, skip=()):
    """Return drift descriptions by regenerating in memory (needs the texture)."""
    problems = []
    for rel, image in build_icons(symbol, enclosing).items():
        path = RES_DIR / rel
        if rel in skip:
            continue
        if not path.exists():
            problems.append(f"{rel} is missing")
            continue
        try:
            existing = Image.open(path).convert("RGBA")
        except OSError:
            problems.append(f"{rel} is not a readable PNG")
            continue
        if existing.tobytes() != image.tobytes():
            problems.append(f"{rel} differs from the generator output")
    return problems


def check_safe_zone(icons):
    """Fail if any masked layer reaches outside the 66dp adaptive safe circle."""
    problems = []
    for rel, image in icons.items():
        if not rel.endswith(("_foreground.png", "_monochrome.png")):
            continue
        alpha = np.array(image.getchannel("A"))
        ys, xs = np.nonzero(alpha > ALPHA_THRESHOLD)
        canvas = image.size[0]
        centre = (canvas - 1) / 2
        radius = np.sqrt(((xs - centre) ** 2 + (ys - centre) ** 2).max()) / canvas
        if radius > SAFE_RADIUS:
            problems.append(
                f"{rel} reaches {radius * 100:.1f}% of the canvas radius, "
                f"outside the {SAFE_RADIUS * 100:.1f}% safe circle"
            )
    return problems


def main():
    if "--check" in sys.argv[1:]:
        problems, drifted = check_manifest()
        if SOURCE.exists():
            symbol, enclosing = load_symbol()
            problems += check_pixels(symbol, enclosing, skip=drifted)
            problems += check_safe_zone(build_icons(symbol, enclosing))
        if problems:
            print("Launcher icons are out of date:", file=sys.stderr)
            for problem in problems:
                print(f"  {problem}", file=sys.stderr)
            print(
                "Run 'python scripts/generate_launcher_icons.py' and commit the result.",
                file=sys.stderr,
            )
            return 1
        if not SOURCE.exists():
            print(
                "loading_symbol.png is absent (gitignored Bethesda asset); "
                "verified against scripts/launcher_icon_manifest.json only."
            )
        print("Launcher icons match the generator output.")
        return 0

    symbol, enclosing = load_symbol()
    cx, cy, radius = enclosing
    print(
        f"source: {SOURCE.relative_to(REPO_ROOT)} -> {symbol.size[0]}x{symbol.size[1]}, "
        f"enclosing circle r={radius:.1f} at ({cx:.1f}, {cy:.1f})"
    )
    icons = build_icons(symbol, enclosing)
    problems = check_safe_zone(icons)
    if problems:
        for problem in problems:
            print(f"unsafe: {problem}", file=sys.stderr)
        return 1
    for rel, image in icons.items():
        target = RES_DIR / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        image.save(target, optimize=True)
    write_manifest(icons)
    for density, (legacy_px, adaptive_px) in DENSITIES.items():
        print(f"  mipmap-{density}: legacy {legacy_px}px, adaptive {adaptive_px}px")
    print(f"manifest: {MANIFEST.relative_to(REPO_ROOT)} ({len(icons)} icons)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
