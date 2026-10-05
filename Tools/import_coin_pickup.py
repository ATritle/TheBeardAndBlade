"""Import the existing RPG Sound Pack recording as a dedicated coin cue.
Source/license attribution is retained in AudioSource/Foley/CREDITS.md.
"""
import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir())
source = root / 'AudioSource/Foley/Prepared/CoinPickup.wav'
task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = '/Game/Audio'
task.destination_name = 'CoinPickup'
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = unreal.load_asset('/Game/Audio/CoinPickup')
assert sound is not None
sound.set_editor_property('looping', False)
unreal.EditorAssetLibrary.save_loaded_asset(sound)
unreal.log('COIN_PICKUP_IMPORT_COMPLETE')
