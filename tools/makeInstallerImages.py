from pathlib import Path

from PIL import Image

root = Path(__file__).resolve().parent.parent
assets = root / "assets"
base = (0x18, 0x1B, 0x1D)
icon = Image.open(assets / "ankhIcon.png").convert("RGBA")


def composed(width, height, iconSize):
  canvas = Image.new("RGBA", (width, height), base + (255,))
  scaled = icon.resize((iconSize, iconSize), Image.LANCZOS)
  canvas.alpha_composite(scaled, ((width - iconSize) // 2, (height - iconSize) // 2))
  return canvas.convert("RGB")


composed(164, 314, 112).save(assets / "installerSide.bmp")
composed(55, 58, 44).save(assets / "installerSmall.bmp")
print("ok")
