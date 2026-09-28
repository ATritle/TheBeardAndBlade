import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir())
for name in ('AtlasMapIcon', 'AtlasTitle', 'CurrencyCoin'):
    task = unreal.AssetImportTask()
    task.filename = str(root / 'ArtSource/Exploration' / (name + '.png'))
    task.destination_path = '/Game/Art/V2'
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    assert task.imported_object_paths, name
    texture = unreal.load_asset(task.imported_object_paths[0])
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('filter', unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('never_stream', True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('ATLAS_UI_IMPORT_COMPLETE')
