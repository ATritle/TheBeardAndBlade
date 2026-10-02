"""Separate alpha-safe spectral material; never modifies source hero textures."""
import unreal
path='/Game/Art/V2/M_DeathSpirit'
lib=unreal.MaterialEditingLibrary
material=unreal.load_asset(path)
if material:
    lib.delete_all_material_expressions(material)
else:
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_DeathSpirit','/Game/Art/V2',unreal.Material,unreal.MaterialFactoryNew())
material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided',True)
tex=lib.create_material_expression(material,unreal.MaterialExpressionTextureSampleParameter2D,-650,0)
tex.set_editor_property('parameter_name','SpriteTexture')
tex.set_editor_property('texture',unreal.load_asset('/Game/Art/V2/Locomotion_Walk_4_2'))
uv=lib.create_material_expression(material,unreal.MaterialExpressionTextureCoordinate,-650,220)
opacity=lib.create_material_expression(material,unreal.MaterialExpressionScalarParameter,-650,360)
opacity.set_editor_property('parameter_name','SpiritOpacity');opacity.set_editor_property('default_value',.65)
color=lib.create_material_expression(material,unreal.MaterialExpressionCustom,-250,0)
color.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
color.set_editor_property('code','float L=dot(C,float3(0.299,0.587,0.114)); return float3(0.50,0.83,1.0)*(0.35+0.65*pow(max(L,0.0),0.4545));')
inp=unreal.CustomInput();inp.set_editor_property('input_name','C');color.set_editor_property('inputs',[inp])
lib.connect_material_expressions(tex,'RGB',color,'C')
lib.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
alpha=lib.create_material_expression(material,unreal.MaterialExpressionCustom,-250,200)
alpha.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
alpha.set_editor_property('code','return A*O*(1.0-0.96*smoothstep(0.53,0.93,UV.y));')
inputs=[]
for name in ['A','O','UV']:
    inp=unreal.CustomInput();inp.set_editor_property('input_name',name);inputs.append(inp)
alpha.set_editor_property('inputs',inputs)
lib.connect_material_expressions(tex,'A',alpha,'A')
lib.connect_material_expressions(opacity,'',alpha,'O')
lib.connect_material_expressions(uv,'',alpha,'UV')
lib.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('DEATH_SPIRIT_MATERIAL_READY')
