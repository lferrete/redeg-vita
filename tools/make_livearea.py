#!/usr/bin/env python3
"""Build the Resident Evil: Degeneration LiveArea artwork.

The source is iTunesArtwork from the user's own legally obtained IPA. It is
written by tools/prepare_data.ps1 to extras/livearea/source_icon.png (or .jpg).

Outputs (tracked):
  extras/livearea/icon0.png      128x128
  extras/livearea/pic0.png       960x544
  extras/livearea/bg0.png        840x500
  extras/livearea/startup.png    280x158

The artwork is derived from the original game and remains the property of its
rights holders. Review the image diff before committing.

Port code Copyright (C) 2026 Leandro Ferrete.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
LIVE = ROOT / "extras" / "livearea"
FONT_BOLD = "C:/Windows/Fonts/arialbd.ttf"
FONT_REGULAR = "C:/Windows/Fonts/arial.ttf"


def load_source():
    path = LIVE / "source_icon.png"
    if path.exists():
        return Image.open(path).convert("RGB")
    raise SystemExit(
        "Source artwork not found. Run tools/prepare_data.ps1 on your legal "
        "IPA (it writes extras/livearea/source_icon.png), then run this script."
    )


def gradient(w, h, top=(22, 6, 7), bottom=(3, 2, 3)):
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(1, h - 1)
        c = tuple(int(top[i] * (1 - t) + bottom[i] * t) for i in range(3))
        d.line([(0, y), (w, y)], fill=c)
    return img


def cover(img, w, h, top_frac=0.5):
    scale = max(w / img.width, h / img.height)
    resized = img.resize((max(1, int(img.width * scale)), max(1, int(img.height * scale))), Image.Resampling.LANCZOS)
    x = (resized.width - w) // 2
    y = int((resized.height - h) * top_frac)
    return resized.crop((x, y, x + w, y + h))


def red_glow(size, color=(120, 8, 10), alpha=95, blur=42):
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    w, h = size
    d.ellipse([-w // 3, -h // 3, w + w // 3, h + h // 3], fill=(*color, alpha))
    return layer.filter(ImageFilter.GaussianBlur(blur))


def vignette(img, strength=0.55):
    w, h = img.size
    vig = Image.radial_gradient("L").resize((w, h), Image.Resampling.BICUBIC)
    vig = Image.eval(vig, lambda p: 255 - int(p * strength))
    return Image.composite(img, Image.new("RGB", (w, h), (0, 0, 0)), vig)


def save_indexed(img, path):
    """LiveArea expects an 8-bit palette PNG, like the system artwork."""
    img = img.convert("RGB")
    q = img.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
    q.save(path, format="PNG", optimize=True)


def make_icon(src):
    im = cover(src, 128, 128)
    im = ImageEnhance.Contrast(im).enhance(1.06)
    im = ImageEnhance.Sharpness(im).enhance(1.12)
    save_indexed(im, LIVE / "icon0.png")


def add_caption(base, title_size, sub_size, x, y):
    d = ImageDraw.Draw(base)
    try:
        title_font = ImageFont.truetype(FONT_BOLD, title_size)
        sub_font = ImageFont.truetype(FONT_BOLD, sub_size)
        small_font = ImageFont.truetype(FONT_REGULAR, max(10, sub_size // 2))
    except OSError:
        title_font = sub_font = small_font = ImageFont.load_default()

    d.text((x + 2, y + 2), "RESIDENT EVIL", font=title_font,
           fill=(0, 0, 0), stroke_width=2, stroke_fill=(0, 0, 0))
    d.text((x, y), "RESIDENT EVIL", font=title_font,
           fill=(240, 232, 224), stroke_width=2, stroke_fill=(10, 4, 4))
    d.text((x + 1, y + int(title_size * 0.82)), "DEGENERATION", font=sub_font,
           fill=(200, 176, 172), stroke_width=1, stroke_fill=(10, 4, 4))
    line_y = y + int(title_size * 1.48)
    d.line([(x + 2, line_y), (x + int(title_size * 4.2), line_y)],
           fill=(160, 28, 24), width=max(2, title_size // 22))
    d.text((x + 2, line_y + max(6, sub_size // 3)), "PS VITA",
           font=small_font, fill=(180, 170, 165))


def make_pic0(src):
    w, h = 960, 544
    # Full-bleed 16:9 crop from the top of the key art (faces, not the logo).
    base = cover(src, w, h, 0.0).convert("RGBA")
    base = ImageEnhance.Brightness(base).enhance(0.86)
    base = ImageEnhance.Contrast(base).enhance(1.04)

    shade = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shade)
    for y in range(h):
        a = int(155 * max(0, (y - int(h * 0.55)) / max(1, h - int(h * 0.55))))
        sd.line([(0, y), (w, y)], fill=(0, 0, 0, a))
    base = Image.alpha_composite(base, shade)
    base = vignette(base.convert("RGB"), 0.35).convert("RGBA")

    add_caption(base, 54, 24, 48, 365)
    save_indexed(base, LIVE / "pic0.png")


def make_bg0(src):
    w, h = 840, 500
    # Full-bleed crop for the gate background, no square/4:3 centre image.
    base = cover(src, w, h, 0.0).convert("RGBA")
    base = ImageEnhance.Brightness(base).enhance(0.86)
    base = ImageEnhance.Contrast(base).enhance(1.04)

    shade = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shade)
    for y in range(h):
        a = int(155 * max(0, (y - int(h * 0.55)) / max(1, h - int(h * 0.55))))
        sd.line([(0, y), (w, y)], fill=(0, 0, 0, a))
    base = Image.alpha_composite(base, shade)
    base = vignette(base.convert("RGB"), 0.35).convert("RGBA")

    add_caption(base, 46, 21, 40, 326)
    save_indexed(base, LIVE / "bg0.png")


def make_startup(src):
    w, h = 280, 158
    # 280x158 is the Vita LiveArea startup-image slot, i.e. 16:9. Use a
    # sharp widescreen crop of the upper key art (faces, not the logo).
    wide_h = max(1, int(src.width * h / w))
    top = max(0, min(src.height - wide_h, int(src.height * 0.04)))
    wide = src.crop((0, top, src.width, top + wide_h)).resize((w, h), Image.Resampling.LANCZOS)
    base = wide.convert("RGBA")
    base = ImageEnhance.Brightness(base).enhance(0.62)
    base = ImageEnhance.Contrast(base).enhance(1.05)

    shade = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shade)
    sd.rectangle([0, 0, w, 58], fill=(0, 0, 0, 150))
    sd.rectangle([0, h - 30, w, h], fill=(0, 0, 0, 165))
    base = Image.alpha_composite(base, shade)

    d = ImageDraw.Draw(base)
    try:
        title_font = ImageFont.truetype(FONT_BOLD, 18)
        sub_font = ImageFont.truetype(FONT_BOLD, 10)
        small_font = ImageFont.truetype(FONT_REGULAR, 9)
    except OSError:
        title_font = sub_font = small_font = ImageFont.load_default()

    d.text((12, 9), "RESIDENT EVIL", font=title_font, fill=(235, 225, 215))
    d.text((13, 30), "DEGENERATION", font=sub_font, fill=(200, 180, 175))
    d.line([(13, 44), (120, 44)], fill=(170, 28, 24), width=2)
    d.text((12, 137), "PS VITA", font=small_font, fill=(190, 180, 175))

    save_indexed(base, LIVE / "startup.png")


def main():
    LIVE.mkdir(parents=True, exist_ok=True)
    src = load_source()
    make_icon(src)
    make_pic0(src)
    make_bg0(src)
    make_startup(src)
    print("LiveArea art generated in", LIVE)


if __name__ == "__main__":
    main()
