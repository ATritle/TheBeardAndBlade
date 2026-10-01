"""Import every reviewed v2 atlas without replacing the existing Keep enemies."""
import json
import os
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir())
base=root/"ArtSource/EnemyExpansion"
inventory=json.loads((base/"HomePCHandoff-v2/inventory.json").read_text())
total=0
for enemy in inventory["enemies"]:
    name=enemy["folder"]
    only=os.environ.get("EXPANSION_V2_ENEMY")
    if only and name not in only.split(","):
        continue
    records=json.loads((base/name/"runtime-v2/runtime-manifest.json").read_text())
    destination="/Game/Art/EnemyExpansion/"+name
    for record in records:
        asset=record["name"]
        task=unreal.AssetImportTask()
        task.filename=str(root/"Content/Art/EnemyExpansion"/name/(asset+".png"))
        task.destination_path=destination
        task.automated=True
        task.replace_existing=True
        task.save=False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture=unreal.load_asset(destination+"/"+asset)
        assert texture,asset
        for key,value in [
            ("filter",unreal.TextureFilter.TF_NEAREST),
            ("mip_gen_settings",unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
            ("compression_settings",unreal.TextureCompressionSettings.TC_EDITOR_ICON),
            ("never_stream",True),
            ("lod_group",unreal.TextureGroup.TEXTUREGROUP_UI)]:
            texture.set_editor_property(key,value)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
        total+=1
    unreal.log(f"EXPANSION_V2_IMPORTED {name} {len(records)} atlases")
    unreal.SystemLibrary.collect_garbage()
unreal.log(f"EXPANSION_V2_IMPORT_COMPLETE {total}")
