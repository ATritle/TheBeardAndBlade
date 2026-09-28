"""Prepare licensed recordings, not synthesized replacements. See AudioSource/Foley/CREDITS.md."""
from pathlib import Path
import json
import numpy as np
import soundfile as sf

root = Path(__file__).resolve().parents[1]
source = root / "AudioSource/Foley"
output = source / "Prepared"
output.mkdir(exist_ok=True)
selections = {}
for i, side in enumerate(("L1", "L2", "L3", "R1", "R2", "R3")):
    selections[f"FoleyStep{i}"] = (source / f"Footsteps/Fantozzi-footsteps/flac/Fantozzi-Stone{side}.flac", .50)
for i, number in enumerate((5, 7, 8, 9)):
    selections[f"FoleySword{i}"] = (source / f"Swishes/swishes/swish-{number}.wav", .75)
for i, number in enumerate((1, 3)):
    selections[f"FoleyEquip{i}"] = (source / f"Weapons/sfx/seax-unsheathe-{number:02}.wav", .60)
expansion = source / "Expansion"
inventory = expansion / "Inventory/inventory_sound_effects"
rpg = expansion / "RPG/RPG Sound Pack"
monsters = expansion / "Monsters/monster"
selections.update({
    "FoleyStep6": (expansion / "Steps/stone01.ogg", .50),
    "FoleyDoor0": (rpg / "world/door.wav", .55),
    "FoleyInventoryOpen0": (inventory / "leather_inventory.wav", .40),
    "FoleyInventoryClose0": (inventory / "cloth-inventory.wav", .35),
    "FoleyPaper0": (inventory / "turn_page.wav", .40),
    "FoleyUI0": (rpg / "inventory/wood-small.wav", .32),
    "FoleyRoll0": (rpg / "inventory/cloth-heavy.wav", .45),
    "FoleyChest0": (rpg / "world/door.wav", .50),
    "FoleyMagic0": (rpg / "battle/magic1.wav", .55),
    "FoleyPortal0": (rpg / "battle/magic1.wav", .45),
    "FoleyExplosion0": (expansion / "Explosion.ogg", .75),
    "FoleyTeaSplash0": (expansion / "Explosion.ogg", .60),
    "FoleyFlashBang0": (expansion / "Explosion.ogg", .70),
    "FoleyHit0": (rpg / "NPC/beetle/bite-small.wav", .55),
})
for i, name in enumerate(("grunt1", "grunt2")):
    selections[f"FoleySpawn{i}"] = (monsters / f"{name}.wav", .40)
for i, name in enumerate(("deathb", "deathd", "deathe", "deathr", "deaths")):
    selections[f"FoleyEnemyDeath{i}"] = (monsters / f"{name}.wav", .55)
for i, name in enumerate(("painb", "paind", "paine", "pains")):
    selections[f"FoleyEnemyPain{i}"] = (monsters / f"{name}.wav", .45)
for i in range(5):
    selections[f"FoleyPropBreak{i}"] = (expansion / f"Break/impcrunch/impactcrunch{i+1:02}.mp3.flac", .65)
for i, name in enumerate(("swing", "swing2", "swing3")):
    selections[f"FoleyThrow{i}"] = (rpg / f"battle/{name}.wav", .45)
report = {}
voices = expansion / "Voices/Wav"
for i in range(3):
    selections[f"FoleyHumanPain{i}"] = (voices / f"Human/Human_Good_{i:02}.wav", .45)
    selections[f"FoleyHumanDeath{i}"] = (voices / f"Human/Human_DyingBreath_{i:02}.wav", .55)
for i in range(2):
    selections[f"FoleyHumanSpawn{i}"] = (voices / f"Human/Human_Evil_{i+1:02}.wav", .35)
    selections[f"FoleyDronePain{i}"] = (voices / f"Robot/Robot2_{i:02}.wav", .40)
    selections[f"FoleyDroneDeath{i}"] = (voices / f"Robot/Robot2_{i+2:02}.wav", .50)
selections["FoleyDroneFlight0"] = (expansion / "Motor.flac", .35)
selections["FoleyGrenade0"] = (expansion / "Explosion.ogg", .70)
for i, path in enumerate(("AR-15/D_32P.wav", "AR-15/D_24P.wav", "AK-47/C_28P.wav")):
    selections[f"FoleyRifle{i}"] = (expansion / "Firearms/Prepared SFX Library" / path, .65)
preview = []
for name, (path, peak) in selections.items():
    samples, rate = sf.read(path, always_2d=True)
    samples = samples.mean(axis=1)
    samples -= samples.mean()
    if name.startswith("FoleyStep"):
        samples = np.convolve(samples, np.array([1, 2, 3, 2, 1])/9, mode="same")
    active = np.flatnonzero(abs(samples) > max(abs(samples).max() * .004, .0001))
    assert len(active), path
    samples = samples[max(0, active[0] - int(rate*.004)):min(len(samples), active[-1] + int(rate*.025))]
    if name.startswith(("FoleyHuman", "FoleyDrone", "FoleyRifle", "FoleyGrenade")):
        limit = 1.1 if name.startswith("FoleyRifle") else 2.2 if name=="FoleyDroneFlight0" else 2.8
        samples = samples[:int(rate*limit)]
    if name in ("FoleyExplosion0", "FoleyTeaSplash0", "FoleyFlashBang0"):
        limit = {"FoleyExplosion0": 2.2, "FoleyTeaSplash0": 1.25, "FoleyFlashBang0": .65}[name]
        samples = samples[:int(rate*limit)]
    fade = min(int(rate*(.04 if any(s in name for s in ("Explosion", "Splash", "FlashBang", "HumanDeath", "Drone", "Grenade")) else .004)), len(samples)//4)
    samples[:fade] *= np.linspace(0, 1, fade)
    samples[-fade:] *= np.linspace(1, 0, fade)
    samples *= peak / max(abs(samples).max(), 1e-9)
    sf.write(output / f"{name}.wav", samples, rate, subtype="PCM_16")
    assert np.isfinite(samples).all() and abs(samples).max() < 1
    report[name] = {"source": str(path.relative_to(root)).replace("\\", "/"),
                    "seconds": len(samples)/rate, "sample_rate": rate,
                    "peak": float(abs(samples).max()), "loop": False}
    # A review reel, not used by the game: steps, swings, then gear handling.
    resampled = np.interp(np.arange(round(len(samples)*44100/rate))*rate/44100, np.arange(len(samples)), samples)
    preview.extend((resampled, np.zeros(15435)))
sf.write(output / "FoleyPreview.wav", np.concatenate(preview), 44100, subtype="PCM_16")
(source / "manifest.json").write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
