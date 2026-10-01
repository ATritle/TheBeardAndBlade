"""Editable geometric modeling study, not a conversion of the concept into a mesh.
Run with Blender --background --python. No gameplay assets are replaced.
"""
import bpy, math, random
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/InventoryExpansion/Model-v1'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
random.seed(12)
def material(name,color,metal=0):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=.8
    return m
skin=material('Warm skin',(.57,.29,.17));skinlight=material('Skin highlight',(.69,.39,.24))
leather=material('Chestnut leather',(.19,.075,.031));edge=material('Leather raised seams',(.31,.15,.064))
dark=material('Charcoal trousers',(.048,.042,.043));green=material('Forest cloak',(.039,.105,.035));greenlight=material('Cloak raised folds',(.07,.155,.051))
iron=material('Worn steel',(.3,.32,.32),.65);ironlight=material('Steel edges',(.47,.48,.44),.7)
gold=material('Antique brass',(.54,.31,.075),.65);black=material('Black spectacle frames',(.012,.01,.008))
ginger=material('Ginger beard',(.36,.085,.011));gingerlight=material('Beard highlights',(.53,.16,.022));fur=material('Warm fur',(.58,.43,.26))
parts=[]
def finish(o,name,mat):
    o.name=name;o.data.materials.append(mat);parts.append(o);return o
def ell(name,loc,scale,mat,segments=12,rings=8):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,location=loc)
    o=bpy.context.object;o.scale=scale;return finish(o,name,mat)
def rod(name,a,b,r,mat,r2=None,vertices=10):
    v=Vector(b)-Vector(a);bpy.ops.mesh.primitive_cone_add(vertices=vertices,radius1=r,radius2=r if r2 is None else r2,depth=v.length,location=(Vector(a)+Vector(b))/2)
    o=bpy.context.object;o.rotation_euler=v.to_track_quat('Z','Y').to_euler();return finish(o,name,mat)
def box(name,loc,scale,mat,bevel=.01):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Small tailored edge','BEVEL');mod.width=bevel;mod.segments=1
        bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
    return finish(o,name,mat)
def mesh(name,verts,faces,mat):
    m=bpy.data.meshes.new(name);m.from_pydata(verts,[],faces);m.update();o=bpy.data.objects.new(name,m);bpy.context.collection.objects.link(o);return finish(o,name,mat)
def ring(name,loc,major,minor,mat,rot=(math.pi/2,0,0)):
    bpy.ops.mesh.primitive_torus_add(major_segments=16,minor_segments=4,location=loc,rotation=rot,major_radius=major,minor_radius=minor)
    return finish(bpy.context.object,name,mat)

# Forward is -Y. Separate components remain editable for a later rig.
ell('Pelvis',(0,0,.83),(.205,.125,.17),dark)
ell('Leather torso',(0,0,1.14),(.235,.135,.29),leather)
rod('Neck',(0,0,1.36),(0,0,1.48),.075,skin)
ell('Bald cranium',(0,-.006,1.605),(.104,.095,.137),skinlight,20,12)
ell('Face',(0,-.071,1.56),(.079,.054,.087),skin,16,10)
ell('Nose',(0,-.132,1.575),(.022,.026,.036),skinlight)
for side in (-1,1):
    ell('Ear',(side*.103,-.008,1.586),(.021,.015,.036),skinlight)
    ell('Brow',(side*.045,-.109,1.611),(.036,.013,.011),ginger)
    ell('Eye white',(side*.044,-.119,1.598),(.023,.009,.012),fur)
    ell('Eye pupil',(side*.044,-.128,1.598),(.007,.004,.008),black)
    ring('Round spectacle lens rim',(side*.046,-.128,1.598),.032,.004,black)
    rod('Spectacle temple',(side*.078,-.123,1.605),(side*.102,.005,1.606),.0035,black)
rod('Spectacle bridge',(-.014,-.134,1.604),(.014,-.134,1.604),.0035,black)
ell('Beard mass',(0,-.102,1.481),(.087,.06,.114),ginger,14,10)
for i in range(17):
    x=(i%7-3)*.022;z=1.515-(i//7)*.048
    rod('Sculpted beard strand',(x,-.143,z),(x*.68,-.148,z-.073),.012,gingerlight if i%3==0 else ginger,.002,6)
for side in (-1,1):
    ell('Moustache',(side*.028,-.15,1.546),(.036,.013,.012),gingerlight)
    hip=(side*.122,0,.83);knee=(side*.139,-.003,.52);ankle=(side*.16,-.005,.19)
    rod('Trouser thigh',hip,knee,.09,dark,.071)
    rod('Leather boot shaft',ankle,knee,.062,leather,.077)
    ell('Boot foot',(side*.165,-.077,.08),(.075,.15,.066),leather)
    box('Boot sole',(side*.165,-.08,.033),(.15,.29,.035),dark)
    rod('Fur boot cuff',(side*.139,0,.485),(side*.139,0,.53),.081,fur)
    for j in range(4):
        z=.22+j*.065
        rod('Boot strap',(side*.158,0,z),(side*.157,0,z+.018),.067,edge)
        box('Boot buckle',(side*.205,-.047,z+.01),(.022,.014,.025),gold,.002)
    for j in range(6):
        z=.17+j*.044
        rod('Cross lacing',(side*.16-.033,-.069,z),(side*.16+.033,-.069,z+.035),.003,fur,vertices=6)
        rod('Cross lacing',(side*.16+.033,-.07,z),(side*.16-.033,-.07,z+.035),.003,fur,vertices=6)
    shoulder=(side*.242,0,1.32);elbow=(side*.287,-.008,1.10);wrist=(side*.315,-.04,.91)
    rod('Chainmail upper arm',elbow,shoulder,.067,iron,.078)
    for j in range(7):
        for k in range(8):
            a=k*math.tau/8;x=side*(.247+j*.005)+math.cos(a)*.066;y=math.sin(a)*.066
            ell('Chainmail scale',(x,y,1.30-j*.022),(.01,.008,.014),ironlight if (j+k)%3==0 else iron,6,4)
    ell('Pauldron',(side*.235,0,1.336),(.107,.143,.073),iron)
    for j in range(3):
        ell('Layered shoulder plate',(side*(.252+j*.012),-.001,1.313-j*.025),(.09-j*.008,.121-j*.008,.027),ironlight if j==2 else iron)
    rod('Fur sleeve cuff',(side*.281,-.006,1.095),(side*.277,-.006,1.137),.071,fur)
    rod('Bracer',wrist,elbow,.047,leather,.065)
    for j in range(3):
        z=.94+j*.056;rod('Bracer band',(side*(.31-j*.007),-.03,z),(side*(.31-j*.007),-.03,z+.016),.054+j*.003,edge)
    ell('Hand',(side*.324,-.045,.855),(.039,.027,.06),skinlight)
    for j in range(4):
        x=side*(.303+j*.013);rod('Finger',(x,-.059,.839),(x+side*.005,-.063,.793+(j%2)*.005),.007,skin,.006,6)
    rod('Thumb',(side*.301,-.061,.876),(side*.289,-.083,.835),.01,skinlight,.008)
    # Flared leather coat skirts and fur edging.
    verts=[(side*.035,-.14,.99),(side*.19,-.10,.99),(side*.245,-.055,.735),(side*.072,-.15,.718)]
    o=mesh('Coat skirt panel',verts,[(0,1,2,3)],leather);m=o.modifiers.new('Leather thickness','SOLIDIFY');m.thickness=.016
    rod('Skirt fur hem',verts[2],verts[3],.014,fur)
    box('Belt pouch',(side*.16,-.116,.986),(.095,.043,.072),leather)
    box('Pouch flap',(side*.16,-.144,1.008),(.096,.012,.028),edge)
rod('Belt',(0,0,.968),(0,0,1.025),.205,edge,vertices=16).scale.y=.68
ring('Belt buckle',(0,-.154,.997),.038,.008,gold)
ell('Buckle medallion',(0,-.157,.997),(.032,.009,.032),gold)
for j in range(3):
    z=1.11+j*.065;rod('Tunic gold clasp',(-.027,-.134,z),(.027,-.134,z),.005,gold)
    ell('Clasp button',(.028,-.137,z),(.008,.007,.008),gold)
box('Tunic center seam',(0,-.137,1.19),(.012,.01,.25),edge,.001)
mesh('Green tabard',[(-.045,-.169,.972),(.045,-.169,.972),(.052,-.181,.69),(-.052,-.181,.69)],[(0,1,2,3)],green)
for side in (-1,1):rod('Tabard gold edge',(side*.044,-.174,.96),(side*.051,-.186,.70),.003,gold)
for j in range(5):
    z=.74+j*.035
    rod('Tabard embroidery',(-.022,-.187,z),(0,-.187,z+.02),.0025,gold)
    rod('Tabard embroidery',(0,-.187,z+.02),(.022,-.187,z),.0025,gold)
# Dimensional cape with longitudinal folds and ragged lower hem.
verts=[];faces=[];cols=17;rows=9
for j in range(rows):
    t=j/(rows-1);width=.18+.14*t
    for i in range(cols):
        u=i/(cols-1)*2-1
        z=1.36-t*1.10
        if j==rows-1:z+=.06*((i*7)%5)/4
        verts.append((u*width,.113+.075*t+.035*math.cos(u*math.pi*5)*t,z))
for j in range(rows-1):
    for i in range(cols-1):a=j*cols+i;faces.append((a,a+1,a+cols+1,a+cols))
cape=mesh('Ragged folded green cape',verts,faces,green);cape.data.materials.append(greenlight)
for p in cape.data.polygons:p.material_index=1 if p.index%16 in (2,5,9,12) else 0
mod=cape.modifiers.new('Cloak thickness','SOLIDIFY');mod.thickness=.007
ell('Shoulder green wrap',(0,.009,1.383),(.23,.147,.052),green)
rod('Diagonal cloak fold',(-.18,-.094,1.397),(.16,-.145,1.30),.025,greenlight)
ell('Gold cloak clasp',(-.161,-.124,1.381),(.024,.012,.024),gold)

# Model origin is the floor under the pelvis, meters. Export only character meshes.
for o in bpy.context.selected_objects:o.select_set(False)
for o in parts:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'Adventurer-model-v1.glb'),export_format='GLB',use_selection=True,export_apply=True)
# Actual geometry renders, no generated turnaround frames masquerading as 3D.
world=bpy.data.worlds.new('Studio');bpy.context.scene.world=world;world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.055,.06,.05,1);world.node_tree.nodes['Background'].inputs[1].default_value=.5
def light(name,loc,power,size):
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.name=name;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
light('Warm key',(-2,-3,4),170,3);light('Soft fill',(2,-1,2),70,2);light('Cape rim',(0,2,3),160,2)
bpy.ops.object.camera_add(location=(2,-4,1.8));camera=bpy.context.object;camera.data.type='ORTHO';camera.data.ortho_scale=1.95;bpy.context.scene.camera=camera
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=480;scene.render.resolution_y=640;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.film_transparent=True
scene.view_settings.view_transform='Standard'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Adventurer-model-v1.blend'))
for name,angle in [('front',-20),('side',90),('back',160)]:
    a=math.radians(angle);camera.location=(math.sin(a)*4,-math.cos(a)*4,1.4);camera.rotation_euler=(Vector((0,0,.87))-camera.location).to_track_quat('-Z','Y').to_euler()
    scene.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
print('INVENTORY_MODEL_STUDY_COMPLETE')
