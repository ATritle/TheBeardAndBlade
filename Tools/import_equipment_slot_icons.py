"""Reuse existing item art as muted slot guides; keep actual loot textures unchanged."""
import unreal
for name,index in [('Weapon',0),('Armor',26),('Amulet',36),('Ring',52)]:
    source=f'/Game/Art/V2/Loot_{index}'
    destination=f'/Game/Art/V2/InventorySlot{name}'
    tex=unreal.load_asset(destination) if unreal.EditorAssetLibrary.does_asset_exist(destination) else unreal.EditorAssetLibrary.duplicate_asset(source,destination)
    assert tex,destination
    tex.set_editor_property('adjust_saturation',0.0)
    tex.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property('never_stream',True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
unreal.log('EQUIPMENT_SLOT_ICONS_COMPLETE')
