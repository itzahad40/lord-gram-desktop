#!/usr/bin/env python3
"""Generate Lord Gram launcher icons from one logo image.
usage: make_icons.py <telegram-src-dir> <logo.jpg|png>
Writes legacy PNGs, an adaptive-icon foreground and the adaptive xml files into TMessagesProj/src/main/res.
"""
import pathlib
import sys

from PIL import Image, ImageDraw

DENSITY = {"mdpi": 1.0, "hdpi": 1.5, "xhdpi": 2.0, "xxhdpi": 3.0, "xxxhdpi": 4.0}


def square_logo(path):
    im = Image.open(path).convert("RGB")
    side = min(im.size)
    left, top = (im.width - side) // 2, (im.height - side) // 2
    return im.crop((left, top, left + side, top + side))


def circle(im):
    mask = Image.new("L", im.size, 0)
    ImageDraw.Draw(mask).ellipse((0, 0, im.width - 1, im.height - 1), fill=255)
    out = im.convert("RGBA")
    out.putalpha(mask)
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    res = pathlib.Path(sys.argv[1]) / "TMessagesProj/src/main/res"
    logo = square_logo(sys.argv[2])
    if not res.is_dir():
        sys.exit(f"make_icons: {res} not found")

    for name, d in DENSITY.items():
        out = res / f"mipmap-{name}"
        out.mkdir(parents=True, exist_ok=True)
        legacy = round(48 * d)
        img = logo.resize((legacy, legacy), Image.LANCZOS)
        img.convert("RGBA").save(out / "ic_launcher.png")
        circle(img).save(out / "ic_launcher_round.png")

        # adaptive foreground: 108dp canvas, logo 70dp in the middle (keeps it inside every mask shape)
        canvas = round(108 * d)
        inner = round(70 * d)
        fg = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
        fg.paste(logo.resize((inner, inner), Image.LANCZOS).convert("RGBA"), ((canvas - inner) // 2, (canvas - inner) // 2))
        fg.save(out / "lg_icon_foreground.png")

    (res / "drawable").mkdir(parents=True, exist_ok=True)
    (res / "drawable/lg_icon_bg.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<shape xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <solid android:color="#000000" />\n'
        '</shape>\n')

    adaptive = (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@drawable/lg_icon_bg" />\n'
        '    <foreground android:drawable="@mipmap/lg_icon_foreground" />\n'
        '</adaptive-icon>\n')
    (res / "mipmap-anydpi-v26").mkdir(parents=True, exist_ok=True)
    (res / "mipmap-anydpi-v26/ic_launcher.xml").write_text(adaptive)
    (res / "mipmap-anydpi-v26/ic_launcher_round.xml").write_text(adaptive)
    print("make_icons ok")


main()
