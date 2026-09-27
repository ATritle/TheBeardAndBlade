import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
t=unreal.AssetImportTask()
t.filename=str(root/'ArtSource/Destructibles/DestructiblesAtlas.png')
t.destination_path='/Game/Art/V2'
t.automated=True;t.replace_existing=True;t.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
assert t.imported_object_paths
tex=unreal.load_asset(t.imported_object_paths[0])
tex.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
tex.set_editor_property('never_stream',True)
unreal.EditorAssetLibrary.save_loaded_asset(tex)
unreal.log('DESTRUCTIBLES_IMPORT_COMPLETE')
