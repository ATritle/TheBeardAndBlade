import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
task=unreal.AssetImportTask()
task.filename=str(root/'ArtSource/Bosses/IronMatriarch/runtime-v1/Iron_flying_slam_front.png')
task.destination_path='/Game/Art/IronMatriarch'
task.destination_name='Iron_flying_slam_front'
task.automated=True;task.replace_existing=True;task.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
assert task.imported_object_paths
asset=unreal.load_asset(task.imported_object_paths[0])
asset.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
asset.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
asset.set_editor_property('never_stream',True)
asset.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
unreal.EditorAssetLibrary.save_loaded_asset(asset)
assert asset.get_editor_property('filter')==unreal.TextureFilter.TF_NEAREST
unreal.log('IRON_FRONT_ONLY_IMPORT_OK sharp filtering, lossless, resident')
