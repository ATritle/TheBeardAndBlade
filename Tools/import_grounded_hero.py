import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
source=root/'ArtSource/HeroGroundedV3'
names=json.loads((source/'manifest.json').read_text())
assert len(names)==1072
tasks=[]
for name in names:
    task=unreal.AssetImportTask();task.filename=str(source/f'{name}.png')
    task.destination_path='/Game/Art/V2';task.automated=True;task.replace_existing=True;task.save=True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in names:
    texture=unreal.load_asset('/Game/Art/V2/'+name);assert texture,name
    for key,value in [('filter',unreal.TextureFilter.TF_NEAREST),('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),('never_stream',True),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)]:texture.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log(f'GROUNDED_IMPORT_COMPLETE {len(names)} assets')
