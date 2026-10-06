from pathlib import Path
import json
import unreal
root=Path(unreal.Paths.project_dir())/'ArtSource/HeroFullBodyV1/runtime-test'
names=json.loads((root/'manifest.json').read_text())
assert len(names)==152
tasks=[]
for name in names:
    t=unreal.AssetImportTask();t.filename=str(root/f'{name}.png')
    t.destination_path='/Game/Art/V2';t.automated=True;t.replace_existing=True;t.save=True;tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in names:
    t=unreal.load_asset('/Game/Art/V2/'+name);assert t,name
    for key,value in [('filter',unreal.TextureFilter.TF_BILINEAR),('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),('never_stream',True),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)]:t.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(t)
unreal.log('FULLBODY_IMPORT_COMPLETE 152')
