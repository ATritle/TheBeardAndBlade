"""Import transparent, independently animated HUD components; no flattened backdrop."""
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir())
for name in ("OrbFrame", "Slot", "Attack", "Block", "Freedom", "Wrap", "Lightning", "Chassis", "Gem"):
    asset="Hotbar_"+name
    task=unreal.AssetImportTask()
    task.filename=str(root/"ArtSource/UI/Hotbar/v3/components"/(asset+".png"))
    task.destination_path="/Game/Art/UI/Hotbar"
    task.automated=True
    task.replace_existing=True
    task.save=False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset(task.destination_path+"/"+asset)
    assert texture, asset
    for key,value in (
        ("filter",unreal.TextureFilter.TF_BILINEAR),
        ("mip_gen_settings",unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
        ("compression_settings",unreal.TextureCompressionSettings.TC_EDITOR_ICON),
        ("never_stream",True),
        ("lod_group",unreal.TextureGroup.TEXTUREGROUP_UI)):
        texture.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    unreal.log("HOTBAR_IMPORTED "+asset)
unreal.log("HOTBAR_IMPORT_COMPLETE 9 transparent components")
