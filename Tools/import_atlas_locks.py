"""Import the four directional chain locks without changing their generated alpha."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
for direction in 'NESW':
    name='AtlasLock_'+direction
    task=unreal.AssetImportTask()
    task.filename=str(root/'ArtSource/Exploration/LocksV1'/(name+'.png'))
    task.destination_path='/Game/Art/V2'
    task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    assert task.imported_object_paths,name
    texture=unreal.load_asset(task.imported_object_paths[0])
    for key,value in [('filter',unreal.TextureFilter.TF_BILINEAR),('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),('never_stream',True),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)]:
        texture.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('ATLAS_LOCK_IMPORT_COMPLETE 4')
