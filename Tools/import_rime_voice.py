import unreal,json
from pathlib import Path
source=Path(unreal.Paths.project_dir())/'AudioSource/Foley/Rime'
manifest=json.loads((source/'manifest.json').read_text())
assert len(manifest)==4
assert 'FoleyRimeAttack1' not in manifest and 'FoleyRimeAttack2' not in manifest
selected=[name for name in manifest if '-RimeAttackOnly' not in unreal.SystemLibrary.get_command_line() or name.startswith('FoleyRimeAttack')]
for name in selected:
    task=unreal.AssetImportTask()
    task.filename=str(source/f'{name}.wav');task.destination_path='/Game/Audio'
    task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound=unreal.load_asset('/Game/Audio/'+name);assert sound,name
    sound.set_editor_property('looping',False)
    unreal.EditorAssetLibrary.save_loaded_asset(sound)
    assert .1<sound.get_editor_property('duration')<1.5,name
unreal.log(f'RIME_VOICE_IMPORT_COMPLETE count={len(selected)}')
