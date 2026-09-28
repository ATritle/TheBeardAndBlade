"""Download source packs referenced in the audio credits; never execute pack contents."""
import html
import re
import urllib.request
import zipfile
import py7zr
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "AudioSource/Foley/Expansion"
root.mkdir(exist_ok=True)
pages = {
    "Firearms": "https://opengameart.org/content/the-free-firearm-sound-library",
    "Voices": "https://opengameart.org/content/voices-sound-effects-library",
    "Motor": "https://opengameart.org/content/enginemechanic-working-sound",
    "Inventory": "https://opengameart.org/content/inventory-sound-effects",
    "Steps": "https://opengameart.org/content/different-steps-on-wood-stone-leaves-gravel-and-mud",
    "Monsters": "https://opengameart.org/content/15-monster-gruntpaindeath-sounds",
    "RPG": "https://opengameart.org/content/rpg-sound-pack",
    "Break": "https://opengameart.org/content/5-break-crunch-impacts",
    "Explosion": "https://opengameart.org/content/explosions-4",
}
for name, page in pages.items():
    if (root / name).exists() or (root / (name + ".ogg")).exists() or (root / (name + ".flac")).exists():
        continue
    body = urllib.request.urlopen(page).read().decode()
    links = [html.unescape(link) for link in re.findall(r'href="([^"]+)"', body)]
    url = next(link for link in links if "/sites/default/files/" in link
               and any(link.lower().endswith(ext) for ext in (".zip", ".7z", ".ogg", ".flac"))
               and (name != "Motor" or link.endswith("loop.flac")))
    ext = Path(url).suffix
    archive = root / (name + ext)
    urllib.request.urlretrieve(url, archive)
    print(name, url, flush=True)
    if ext == ".zip":
        with zipfile.ZipFile(archive) as pack:
            pack.extractall(root / name)
    elif ext == ".7z":
        with py7zr.SevenZipFile(archive) as pack:
            pack.extractall(root / name)
