"""Import the dedicated transparent combat badge; never replace shared UI art."""
import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir())
task = unreal.AssetImportTask()
task.filename = str(root / 'ArtSource/CombatCombo/Combat_Combo.png')
task.destination_path = '/Game/Art/V2'
task.destination_name = 'Combat_Combo'
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset('/Game/Art/V2/Combat_Combo')
assert texture
for key, value in [('filter', unreal.TextureFilter.TF_BILINEAR),
                   ('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
                   ('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON),
                   ('never_stream', True), ('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)]:
    texture.set_editor_property(key, value)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('COMBO_IMPORT_COMPLETE')
