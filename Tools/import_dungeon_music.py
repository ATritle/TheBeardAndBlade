import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
t=unreal.AssetImportTask()
t.filename=str(root/'AudioSource/Music/Gameplay/HitCtrl/MusicDungeon.wav')
t.destination_path='/Game/Audio';t.destination_name='MusicDungeon'
t.automated=True;t.replace_existing=True;t.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
a=unreal.load_asset('/Game/Audio/MusicDungeon');assert a
a.set_editor_property('looping',True)
unreal.EditorAssetLibrary.save_loaded_asset(a)
assert a.get_editor_property('duration')>30 and a.get_editor_property('looping')
unreal.log('DUNGEON_MUSIC_IMPORT_COMPLETE')
