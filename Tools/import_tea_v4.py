import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
files=sorted((root/'ArtSource/Tea/runtime-v4').glob('TeaV4_*.png'))
assert len(files)==18
for p in files:
    t=unreal.AssetImportTask();t.filename=str(p);t.destination_path='/Game/Art/TeaV4';t.destination_name=p.stem
    t.automated=True;t.replace_existing=True;t.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);assert t.imported_object_paths
    asset=unreal.load_asset(t.imported_object_paths[0])
    asset.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
    asset.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    asset.set_editor_property('never_stream',True)
    asset.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log('TEA_V4_IMPORT_COMPLETE 18 dedicated assets; shared TeaFX unchanged')
