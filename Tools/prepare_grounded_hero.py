"""Author additive 32-frame walk/run cycles from the existing character pixels.

User-approved local processing: measured leg masks, two-bone gait, alpha layers.
No original image is overwritten. Body and weapon grip use the same transform.
"""
from pathlib import Path
import json, math, argparse
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'Content/Art/V2'
OUT=ROOT/'ArtSource/HeroGroundedV3'
OUT.mkdir(exist_ok=True)
N=32; S=3; SIZE=480
def load_art(path):
    source=Image.open(path).convert('RGBA')
    canvas=Image.new('RGBA',(SIZE,SIZE));canvas.paste(source,(0,0));return np.array(canvas)
# One unobscured boot/leg in each facing, measured in the original 128px space.
RIG=[
 ((70,86),(70,98),(68,111),[(65,86),(76,88),(76,101),(78,116),(61,117),(62,103)]),
 ((56,84),(49,97),(42,109),[(49,82),(62,86),(55,98),(49,106),(55,113),(49,117),(33,112),(34,104),(42,95)]),
 ((71,84),(80,95),(85,104),[(65,82),(77,84),(84,94),(89,103),(99,103),(102,110),(87,116),(80,111),(75,102),(67,96)]),
 ((59,84),(60,98),(61,111),[(52,81),(65,84),(67,99),(66,106),(73,109),(71,115),(54,117),(51,111),(52,98)]),
 ((57,85),(58,98),(57,112),[(50,83),(63,84),(65,98),(64,108),(65,117),(50,118),(48,111),(50,98)]),
 ((57,85),(53,98),(48,110),[(51,83),(64,87),(60,97),(53,107),(53,114),(48,117),(38,111),(38,105),(46,102),(48,93)]),
 ((54,85),(52,99),(48,112),[(49,84),(62,88),(58,100),(54,108),(54,116),(39,117),(34,113),(34,108),(44,107),(47,96)]),
 ((64,84),(74,98),(81,110),[(58,82),(71,85),(77,94),(84,101),(89,105),(89,112),(82,117),(75,117),(73,112),(70,108),(66,102),(60,96)])
]
registration=json.loads((ROOT/'ArtSource/HeroLocomotionV2/registration.json').read_text())
hands={r['name']:r['socket'] for r in registration}

def over(dst,src):
    a=src[:,:,3:4].astype(np.float32)/255
    da=dst[:,:,3:4].astype(np.float32)/255
    oa=a+da*(1-a)
    rgb=(src[:,:,:3]*a+dst[:,:,:3]*da*(1-a))/np.maximum(oa,1e-6)
    return np.concatenate((rgb,oa*255),2).clip(0,255).astype(np.uint8)

def limb_mesh(texture,hip,knee,ankle,h,k,a,pitch):
    source=[np.array(hip),np.array(knee),np.array(ankle),np.array(ankle)+[8,0]]
    target=[h,k,a,a+np.array([math.cos(pitch),math.sin(pitch)])*8]
    matrices=[]
    for i in range(3):
        u=source[i+1]-source[i];v=target[i+1]-target[i]
        un=np.array([-u[1],u[0]])/np.linalg.norm(u)*5
        vn=np.array([-v[1],v[0]])/np.linalg.norm(v)*5
        matrices.append(np.linalg.solve(np.column_stack(([source[i],source[i+1],source[i]+un],np.ones(3))),np.array([target[i],target[i+1],target[i]+vn])))
    def map_point(p):
        upper=np.clip((knee[1]+3-p[1])/6,0,1)
        foot_weight=np.clip((p[1]-(ankle[1]-2))/4,0,1)
        weights=[upper*(1-foot_weight),(1-upper)*(1-foot_weight),foot_weight]
        return sum(w*(np.append(p,1)@m) for w,m in zip(weights,matrices))*S
    box=Image.fromarray(texture).getbbox();out=np.zeros_like(texture)
    x0,y0,x1,y1=[v/S for v in box]
    for y in np.arange(math.floor(y0)-1,math.ceil(y1)+1,3):
        for x in np.arange(math.floor(x0)-1,math.ceil(x1)+1,3):
            vertices=[np.array([x,y]),np.array([x+3,y]),np.array([x+3,y+3]),np.array([x,y+3])]
            mapped=[map_point(p) for p in vertices]
            for indices in ((0,1,2),(0,2,3)):
                q=np.array([mapped[i] for i in indices]);s=np.array([vertices[i]*S for i in indices])
                xmin=max(0,int(np.floor(q[:,0].min())));xmax=min(SIZE,int(np.ceil(q[:,0].max()))+1)
                ymin=max(0,int(np.floor(q[:,1].min())));ymax=min(SIZE,int(np.ceil(q[:,1].max()))+1)
                if xmin>=xmax or ymin>=ymax:continue
                matrix=np.column_stack((q,np.ones(3)))
                if abs(np.linalg.det(matrix))<.001:continue
                yy,xx=np.mgrid[ymin:ymax,xmin:xmax]
                bary=np.stack((xx+.5,yy+.5,np.ones_like(xx)),axis=-1)@np.linalg.inv(matrix)
                inside=(bary>=-1e-5).all(axis=-1)
                uv=bary@s;ix=np.clip(uv[:,:,0].astype(int),0,SIZE-1);iy=np.clip(uv[:,:,1].astype(int),0,SIZE-1)
                out[ymin:ymax,xmin:xmax][inside]=texture[iy,ix][inside]
    return out

def complete_hidden_leg(leg,hip,knee,ankle):
    """Extend occluded shaft pixels under the coat; retain the authored boot silhouette."""
    yy,xx=np.mgrid[:SIZE,:SIZE];points=np.stack((xx/S,yy/S),axis=-1)
    interior=np.zeros((SIZE,SIZE),bool)
    for start,end,radius in ((hip,knee,5.0),(knee,ankle,4.6)):
        start=np.array(start);delta=np.array(end)-start
        t=np.clip(((points-start)*delta).sum(2)/(delta@delta),0,1)
        interior|=np.linalg.norm(points-(start+t[:,:,None]*delta),axis=2)<radius
    rgb=leg[:,:,:3].astype(float)
    green=(rgb[:,:,1]>rgb[:,:,0]*1.05)&(rgb[:,:,1]>rgb[:,:,2]*1.18)
    leg[(leg[:,:,3]<160)|green,3]=0
    valid=(leg[:,:,3]>0)
    need=interior&~valid
    sy,sx=np.where(valid);dy,dx=np.where(need)
    # Texture dilation only into measured occluded thigh/shin regions, never the surroundings.
    for start in range(0,len(dx),128):
        qx=dx[start:start+128];qy=dy[start:start+128]
        nearest=np.argmin((qx[:,None]-sx)**2+(qy[:,None]-sy)**2,axis=1)
        leg[qy,qx]=leg[sy[nearest],sx[nearest]]
    return leg

def foot(phase,run):
    stance=.36 if run else .60
    stride=(210 if run else 112)/1.375
    reach=stride*stance/2
    if phase<stance:
        f=reach-stride*phase;lift=0
        pitch=-.12*(1-phase/.12) if phase<.12 else .38*max(0,(phase-(stance-.12))/.12)
    else:
        t=(phase-stance)/(1-stance)
        f=-reach+2*reach*(t*t*(3-2*t))
        lift=(17 if run else 7)*math.sin(math.pi*t)
        pitch=.30*(1-t)-.12*t
    return f,lift,pitch,phase<stance

def project(d,lateral,forward,height):
    a=d*math.pi/4
    foreshorten=1/math.sqrt(math.sin(a)**2+(math.cos(a)/.6)**2)
    return np.array([64+math.cos(a)*lateral+math.sin(a)*forward*foreshorten,
                     116+math.sin(a)*lateral*.6-math.cos(a)*forward*foreshorten-height])

def joints(d,side,phase,run,bob,sole_height):
    f,lift,pitch,planted=foot(phase,run)
    hipheight=34-bob; ankleheight=sole_height+lift
    delta=np.array([f,ankleheight-hipheight]);dist=np.linalg.norm(delta)
    length=max((34-sole_height)*.53,dist/2+.01)
    knee=np.array([0,hipheight])+delta/2+np.array([-delta[1],delta[0]])/max(dist,.001)*math.sqrt(max(0,length*length-dist*dist/4))
    lateral=side*5.5
    return (project(d,lateral,0,hipheight),project(d,lateral,knee[0],knee[1]),
            project(d,lateral,f,ankleheight),pitch,planted)

def body_shift(x,y,d,phase,run,bob):
    upper=np.clip((84-y)/64,0,1)
    swing=math.sin(phase*math.tau)
    lean=(5 if run else 1.1)*math.sin(d*math.pi/4)
    # Relaxed arm counter-swing, small torso counter-rotation, stable pelvis.
    arm=np.clip((np.abs(x-64)-13)/13,0,1)*np.clip((y-43)/20,0,1)*np.clip((84-y)/8,0,1)
    dx=lean*upper+math.cos(d*math.pi/4)*swing*.8*upper+arm*swing*math.sin(d*math.pi/4)*2.2
    dy=bob+arm*swing*np.sign(x-64)*(2.5 if run else 1.5)
    return dx,dy

def upper_only(original,d):
    py=np.arange(SIZE)[:,None]/S;px=np.arange(SIZE)[None,:]/S
    rgb=original[:,:,:3].astype(float)
    green=(rgb[:,:,1]>rgb[:,:,0]*1.05)&(rgb[:,:,1]>rgb[:,:,2]*1.18)&(original[:,:,3]>128)
    cape=np.array(Image.fromarray(green.astype(np.uint8)*255).filter(ImageFilter.MaxFilter(3)))>0
    body=original.copy();body[(py>=82)&~cape,3]=0
    if d in (2,3,4,5,6):
        center=(py>=82)&(py<87)&(abs(px-64)<3)
        body[center]=original[center]
    return body

def prepare(direction=None):
    manifests=[];sockets=[];report=[]
    gy,gx=np.mgrid[:SIZE,:SIZE].astype(np.float32);px=gx/S;py=gy/S
    for d,(hip,knee,ankle,polygon) in enumerate(RIG):
        if direction is not None and d!=direction:continue
        original=load_art(SRC/f'Locomotion_Walk_{d}_0.png')
        mask_image=Image.new('L',(SIZE,SIZE));ImageDraw.Draw(mask_image).polygon([(int(x*S),int(y*S)) for x,y in polygon],fill=255);mask=np.array(mask_image)
        leg=original.copy();leg[:,:,3]=np.minimum(leg[:,:,3],mask)
        leg=complete_hidden_leg(leg,hip,knee,ankle)
        body=upper_only(original,d)
        sole_height=Image.fromarray(leg).getbbox()[3]/S-ankle[1]
        ds=[]
        for run in (False,True):
            frames=[]
            for frame in range(N):
                phase=frame/N
                bob=(2.4 if run else 1.2)*math.sin(phase*math.tau*2)
                limbs=[];contacts=[]
                for side in (-1,1):
                    h,k,a,pitch,planted=joints(d,side,(phase+(0 if side==1 else .5))%1,run,bob,sole_height)
                    theta=pitch*math.sin(d*math.pi/4)
                    layer=limb_mesh(leg,hip,knee,ankle,h,k,a,theta)
                    if side==-1:layer[:,:,:3]=(layer[:,:,:3].astype(float)*.88).astype(np.uint8)
                    limbs.append((a[1],layer));contacts.append({'side':side,'ankle':a.tolist(),'planted':planted})
                canvas=np.zeros_like(original)
                for _,layer in sorted(limbs,key=lambda v:v[0]):canvas=over(canvas,layer)
                leg_name=f'GroundedLegs_{"Run" if run else "Walk"}_{d}_{frame}'
                Image.fromarray(canvas).save(OUT/f'{leg_name}.png');manifests.append(leg_name)
                dx,dy=body_shift(px,py,d,phase,run,bob)
                mx=np.rint(gx-dx*S).astype(int);my=np.rint(gy-dy*S).astype(int)
                valid=(mx>=0)&(mx<SIZE)&(my>=0)&(my<SIZE)
                warped=body[np.clip(my,0,SIZE-1),np.clip(mx,0,SIZE-1)].copy();warped[~valid]=0
                canvas=over(canvas,warped)
                name=f'Grounded_{"Run" if run else "Walk"}_{d}_{frame}'
                Image.fromarray(canvas).save(OUT/f'{name}.png');manifests.append(name)
                hx,hy=hands[f'Locomotion_Walk_{d}_0'];sx,sy=body_shift(hx,hy,d,phase,run,bob)
                ds.append([round(float(hx+sx),3),round(float(hy+sy),3)])
                frames.append(Image.fromarray(canvas));report.append({'name':name,'contacts':contacts})
            preview=[]
            for im in frames:
                b=Image.new('RGBA',(SIZE,SIZE),(30,35,40,255));b.alpha_composite(im);preview.append(b.convert('RGB'))
            preview[0].save(OUT/f'{"Run" if run else "Walk"}{d}.gif',save_all=True,append_images=preview[1:],duration=25 if run else 35,loop=0)
            board=Image.new('RGB',(8*192,4*210),(30,35,40));draw=ImageDraw.Draw(board)
            for f,im in enumerate(frames):
                im=im.resize((192,192),Image.Resampling.NEAREST);x=f%8*192;y=f//8*210;board.paste(im,(x,y),im);draw.text((x+4,y+193),str(f),fill='white')
            board.save(OUT/f'{"Run" if run else "Walk"}{d}-review.png')
        sockets.append(ds)
    if direction is not None:return
    for d in range(8):
        for f in range(6):
            source=f'Athletic_Attack{"Diagonal" if d%2 else "Cardinal"}_{d//2}_{f}'
            original=load_art(SRC/f'{source}.png')
            name='GroundedUpper_'+source
            Image.fromarray(upper_only(original,d)).save(OUT/f'{name}.png');manifests.append(name)
    (OUT/'manifest.json').write_text(json.dumps(manifests,indent=2))
    (OUT/'contacts.json').write_text(json.dumps(report,indent=2))
    rows=[' {'+', '.join('{'+f'{x:.3f}f,{y:.3f}f'+'}' for x,y in row)+'}' for row in sockets]
    (ROOT/'Source/TheBeardAndBlade/HeroGroundedSockets.h').write_text('#pragma once\n#include "CoreMinimal.h"\nnamespace HeroGroundedSockets { inline const FVector2D Hand[8][64]={\n'+',\n'.join(rows)+'\n};}\n')
    print('Prepared',len(manifests),'additive frames')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--direction',type=int);prepare(parser.parse_args().direction)
