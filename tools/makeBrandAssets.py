import io
import re
import sys
from pathlib import Path

from PIL import Image
from playwright.sync_api import sync_playwright

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, r"C:\Onyx\tools")
from playwrightCommon import newPage

svgText = (root / "assets" / "ankhWordmark.svg").read_text(encoding="utf-8")
symbolMatch = re.search(r'<g id="ankh-symbol" fill="#C4A565">(.*?)</g>', svgText, re.S)
if symbolMatch is None:
  raise SystemExit("ankh-symbol group not found")
symbolPaths = symbolMatch.group(1)
base = "#181B1C"
gold = "#C4A565"


def symbolSvg(size):
  return (
    f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="4 0 48 64" fill="{gold}">'
    f"{symbolPaths}</svg>"
  )


def render(page, html, width, height):
  page.set_viewport_size({"width": width, "height": height})
  page.set_content(html)
  return Image.open(io.BytesIO(page.screenshot(omit_background=True, type="png"))).convert("RGBA")


with sync_playwright() as playwright:
  browser = playwright.chromium.launch()
  page = newPage(browser)
  iconSource = render(
    page,
    f'<body style="margin:0;background:transparent"><div style="width:256px;height:256px;display:flex;align-items:center;justify-content:center">{symbolSvg(220)}</div></body>',
    256,
    256,
  )
  side = render(
    page,
    f'<body style="margin:0;background:{base}"><div style="width:164px;height:314px;display:flex;align-items:center;justify-content:center">{symbolSvg(96)}</div></body>',
    164,
    314,
  )
  small = render(
    page,
    f'<body style="margin:0;background:{base}"><div style="width:55px;height:58px;display:flex;align-items:center;justify-content:center">{symbolSvg(40)}</div></body>',
    55,
    58,
  )
  browser.close()

assets = root / "assets"
iconSource.save(assets / "ankh.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
side.convert("RGB").save(assets / "installerSide.bmp")
small.convert("RGB").save(assets / "installerSmall.bmp")
print("ok")
