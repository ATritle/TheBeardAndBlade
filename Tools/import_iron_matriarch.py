import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
source=root/'ArtSource/Bosses/IronMatriarch'
imports=[(p,'/Game/Art/IronMatriarch',p.stem) for p in (source/'runtime-v1').glob('*.png')]
imports += [(source/'intro-v1/character.png','/Game/Art/Intros','IntroIronCharacter'),
            (source/'intro-v1/title.png','/Game/Art/Intros','IntroIronTitle'),
            (root/'ArtSource/Exploration/AtlasChamber7.png','/Game/Art/V2','AtlasChamber7'),
            (root/'ArtSource/Exploration/AtlasChamber7.png','/Game/Art/V2','Arena7'),
            (source/'intro-v1/character.png','/Game/Art/September','BossPortrait_55'),
            (source/'intro-v1/title.png','/Game/Art/September','BossName_55'),
            (source/'intro-v1/intro-breviceps-roar.wav','/Game/Audio','IronIntro')]
imports += [(p,'/Game/Audio',p.stem) for p in (source/'runtime-v1').glob('*.wav')]
for p,dest,name in imports:
    task=unreal.AssetImportTask();task.filename=str(p);task.destination_path=dest;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);assert task.imported_object_paths,p
    asset=unreal.load_asset(task.imported_object_paths[0])
    if p.suffix=='.wav':asset.set_editor_property('looping',False)
    else:
        asset.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST if name=='Iron_flying_slam_front' else unreal.TextureFilter.TF_BILINEAR)
        asset.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        asset.set_editor_property('never_stream',True)
        asset.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log(f'IRON_IMPORT_COMPLETE assets={len(imports)}')
