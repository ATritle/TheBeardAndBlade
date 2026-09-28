"""Download the licensed, author-provided loop; preserve its loop boundaries."""
from pathlib import Path
import urllib.request
import numpy as np
import soundfile as sf

root=Path(__file__).resolve().parents[1]
folder=root/'AudioSource/Music/Gameplay/HitCtrl'
folder.mkdir(parents=True,exist_ok=True)
original=folder/'rpg_ambience_-_dungeon.wav'
if not original.exists():
    urllib.request.urlretrieve('https://opengameart.org/sites/default/files/rpg_ambience_-_dungeon.wav',original)
samples,rate=sf.read(original,always_2d=True)
assert np.isfinite(samples).all()
# Only attenuate hot masters; no silence trimming or fades across the loop seam.
samples*=min(1,.8/max(np.abs(samples).max(),1e-9))
sf.write(folder/'MusicDungeon.wav',samples,rate,subtype='PCM_16')
print('DUNGEON_MUSIC_PREPARED',len(samples)/rate,rate)
