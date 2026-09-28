"""Import additive, point-filtered authored movement frames into UE."""
import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
names=json.loads((root/'ArtSource/HeroLocomotionV2/manifest.json').read_text())
assert len(names)==128, 'All eight walk/run directions are required before integration'
for name in names:
    task=unreal.AssetImportTask()
    task.filename=str(root/'Content/Art/V2'/f'{name}.png')
    task.destination_path='/Game/Art/V2'
    task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset('/Game/Art/V2/'+name)
    assert texture,name
    for key,value in [('filter',unreal.TextureFilter.TF_NEAREST),('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),('never_stream',True),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)]:
        texture.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log(f'LOCOMOTION_V2_IMPORT_COMPLETE {len(names)}')
