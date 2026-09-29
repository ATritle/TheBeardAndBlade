"""Import only curated Rustblade runtime art; no other enemies are modified."""
import json
import re
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
records=json.loads((root/'ArtSource/EnemyExpansion/RustbladeSquire/runtime-v2/runtime-manifest.json').read_text())
selection=re.search(r'-RustbladeImportDirection=(N|NE|E|SE|S|SW|W|NW)(?=\s|$)',unreal.SystemLibrary.get_command_line())
if selection:
    records=[r for r in records if r['name'].split('_')[1]==selection.group(1)]
    assert len(records)==37,'Expected a complete single-direction animation set'
destination='/Game/Art/EnemyExpansion/RustbladeSquire'
for record in records:
    name=record['name']
    task=unreal.AssetImportTask()
    task.filename=str(root/'Content/Art/EnemyExpansion/RustbladeSquire'/f'{name}.png')
    task.destination_path=destination
    task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset(destination+'/'+name)
    assert texture,name
    for key,value in [('filter',unreal.TextureFilter.TF_NEAREST),('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),('never_stream',True),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)]:
        texture.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log(f'RUSTBLADE_IMPORT_COMPLETE {len(records)}')
