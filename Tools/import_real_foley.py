import unreal
import json
from pathlib import Path

source = Path(unreal.Paths.project_dir()) / "AudioSource/Foley"
manifest = json.loads((source / "manifest.json").read_text())
for name, info in manifest.items():
    task = unreal.AssetImportTask()
    task.filename = str(source / "Prepared" / f"{name}.wav")
    task.destination_path = "/Game/Audio"
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset("/Game/Audio/" + name)
    assert sound, name
    sound.set_editor_property("looping", False)
    unreal.EditorAssetLibrary.save_loaded_asset(sound)
    duration = sound.get_editor_property("duration")
    assert .04 < duration < 3, (name, duration)
unreal.log("REAL_FOLEY_IMPORT_COMPLETE count=" + str(len(manifest)))
