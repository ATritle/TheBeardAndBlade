"""Import only the skill-2 icon; never replace the shared tea-throw artwork."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
task = unreal.AssetImportTask()
task.filename = str(root / "ArtSource/UI/Hotbar/TeaSpirit/Hotbar_TeaSpirit.png")
task.destination_path = "/Game/Art/UI/Hotbar"
task.automated = True
task.replace_existing = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset("/Game/Art/UI/Hotbar/Hotbar_TeaSpirit")
assert texture, "Tea Spirit icon import failed"
for key, value in (
    ("filter", unreal.TextureFilter.TF_BILINEAR),
    ("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
    ("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON),
    ("never_stream", True),
    ("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI),
):
    texture.set_editor_property(key, value)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log("TEA_SPIRIT_ICON_IMPORTED")
