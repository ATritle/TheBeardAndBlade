"""Import the dedicated health-potion drink; never replace tea or locomotion."""
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir())
task=unreal.AssetImportTask()
task.filename=str(root/'ArtSource/HeroPotion/Potion_DrinkSheet.png')
task.destination_path='/Game/Art/V2'
task.automated=True
task.replace_existing=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture=unreal.load_asset('/Game/Art/V2/Potion_DrinkSheet')
assert texture
for key,value in (
    ('filter',unreal.TextureFilter.TF_NEAREST),
    ('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
    ('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),
    ('never_stream',True),
    ('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI),
):
    texture.set_editor_property(key,value)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('POTION_DRINK_IMPORTED')
