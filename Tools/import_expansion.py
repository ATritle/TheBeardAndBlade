"""Import curated new enemy frames, leaving the approved Rustblade untouched."""
import json
import os
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
total=0
for name in ['GraveglassSlinger','ChainboundBailiff','CandleHexer','SepulcherLancer']:
    records=json.loads((root/'ArtSource/EnemyExpansion'/name/'runtime-v1/runtime-manifest.json').read_text())
    destination='/Game/Art/EnemyExpansion/'+name
    for record in records:
        if os.environ.get('EXPANSION_IMPORT_PREFIX') and not record['name'].startswith(os.environ['EXPANSION_IMPORT_PREFIX']):continue
        asset=record['name'];task=unreal.AssetImportTask()
        task.filename=str(root/'Content/Art/EnemyExpansion'/name/(asset+'.png'))
        task.destination_path=destination;task.automated=True;task.replace_existing=True;task.save=True
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture=unreal.load_asset(destination+'/'+asset);assert texture,asset
        for key,value in [('filter',unreal.TextureFilter.TF_NEAREST),('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),('never_stream',True),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)]:texture.set_editor_property(key,value)
        unreal.EditorAssetLibrary.save_loaded_asset(texture);total+=1
unreal.log(f'EXPANSION_IMPORT_COMPLETE {total}')
