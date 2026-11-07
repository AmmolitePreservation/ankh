import math
import random
from pathlib import Path

from PIL import Image

output = Path(__file__).resolve().parent.parent / "assets" / "textures"
output.mkdir(parents=True, exist_ok=True)
random.seed(7)

stripWidth = 26
stripLength = 512
cornerSize = 170


def plot(image, x, y, alpha, tone):
  width, height = image.size
  if 0 <= x < width:
    y %= height
    current = image.getpixel((x, y))
    merged = min(255, current[3] + alpha)
    image.putpixel((x, y), (tone, tone - 2, tone - 8, merged))


def addFlecks(image, density):
  width, height = image.size
  for y in range(height):
    for x in range(width):
      if random.random() < density(x, y):
        alpha = random.randint(45, 140)
        tone = random.randint(150, 215)
        plot(image, x, y, alpha, tone)
        if random.random() < 0.35:
          plot(image, x + 1, y, alpha // 2, tone)
        if random.random() < 0.2:
          plot(image, x, y + 1, alpha // 2, tone)


def addScratches(image, count, reach):
  for _ in range(count):
    x = int(abs(random.gauss(0, reach)))
    y = random.randint(0, image.size[1] - 1)
    length = random.randint(5, 26)
    alpha = random.randint(25, 75)
    tone = random.randint(130, 190)
    for step in range(length):
      plot(image, x, y + step, alpha, tone)


strip = Image.new("RGBA", (stripWidth, stripLength), (0, 0, 0, 0))
addFlecks(strip, lambda x, y: 0.26 * math.exp(-x / 5.5) + 0.004)
addScratches(strip, 70, 4.5)
strip.save(output / "edgeGrainStrip.png")

corner = Image.new("RGBA", (cornerSize, cornerSize), (0, 0, 0, 0))
addFlecks(corner, lambda x, y: 0.22 * math.exp(-math.hypot(x, y) / 52.0))
addScratches(corner, 40, 30)
corner.save(output / "edgeGrainCorner.png")
print("ok")
