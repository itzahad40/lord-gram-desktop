import os
import sys

from PIL import Image

src = sys.argv[1]
logo = sys.argv[2]
art = src + "/Telegram/Resources/art"
image = Image.open(logo).convert("RGBA")
side = min(image.size)
left = (image.width - side) // 2
top = (image.height - side) // 2
image = image.crop((left, top, left + side, top + side))

for size in (16, 32, 48, 64, 128, 256, 512):
    for suffix, scale in (("", 1), ("@2x", 2)):
        path = f"{art}/icon{size}{suffix}.png"
        if os.path.exists(path):
            out = image.resize((size * scale, size * scale), Image.LANCZOS)
            out.save(path)

ico = art + "/icon256.ico"
image.resize((256, 256), Image.LANCZOS).save(
    ico,
    sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)],
)
print("icons written from", logo)
