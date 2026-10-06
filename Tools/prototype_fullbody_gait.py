"""Full-body SE gait authoring study. Not imported or approved for gameplay.
All joints move coherently; output is a single complete RGBA frame, no runtime
torso/leg split. Original source art remains untouched.
"""
from pathlib import Path
import sys,math,json
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Intermediate/HeroAuthoring/deps'))
import cv2,numpy as np
from PIL import Image,ImageDraw
ART=ROOT/'ArtSource/HeroFullBodyV1';OUT=ART/'gait-prototype';OUT.mkdir(exist_ok=True)
source=np.array(Image.open(ART/'crops/Walk_3/00.png').convert('RGBA'))
SIZE=560;OFFSET=np.array([90.,50.]);h,w=source.shape[:2]
yy,xx=np.mgrid[:h,:w];pixels=np.stack((xx,yy),axis=-1)
src={
 'rleg':np.array([[173,274],[126,320],[103,384],[127,409]],float),
 'lleg':np.array([[206,277],[224,325],[275,427],[325,433]],float),
 'rarm':np.array([[155,100],[120,175],[105,225],[100,244]],float),
 'larm':np.array([[230,110],[260,165],[289,214],[294,225]],float)}
def segment_distance(points,a,b):
    delta=b-a;t=np.clip(((points-a)*delta).sum(-1)/(delta@delta),0,1)
    return np.linalg.norm(points-(a+t[...,None]*delta),axis=-1)
def chain_distance(points,chain):
    return np.minimum.reduce([segment_distance(points,a,b) for a,b in zip(chain[:-1],chain[1:])])
green=(source[:,:,1]>source[:,:,0]*1.1)&(source[:,:,1]>source[:,:,2]*1.15)
legregion=(yy>273)&~green&~((xx>193)&(xx<249)&(yy<317))
rd=chain_distance(pixels,src['rleg']);ld=chain_distance(pixels,src['lleg'])
masks={'rleg':legregion&(rd<=ld),'lleg':legregion&(ld<rd),
       'rarm':(yy>140)&(yy<275)&(xx<150)&~green,
       'larm':(yy>132)&(yy<255)&(xx>250)&~green}
layers={};body=source.copy()
for name,mask in masks.items():
    tex=source.copy();tex[~mask]=0;layers[name]=tex;body[mask]=0

def similarity(a,b,c,d):
    u=b-a;v=d-c;co=np.dot(u,v)/np.dot(u,u);si=(u[0]*v[1]-u[1]*v[0])/np.dot(u,u)
    R=np.array([[co,-si],[si,co]]);return np.column_stack((R,c-R@a))
def warp_limb(texture,chain,target):
    bbox=Image.fromarray(texture).getbbox();out=np.zeros((SIZE,SIZE,4),np.uint8)
    if not bbox:return out
    x0,y0,x1,y1=bbox
    xs=np.arange(x0-2,x1+9,8);ys=np.arange(y0-2,y1+9,8)
    grid=np.stack(np.meshgrid(xs,ys),-1).reshape(-1,2).astype(float)
    distances=np.stack([segment_distance(grid,a,b) for a,b in zip(chain[:-1],chain[1:])],-1)
    weights=1/(distances+4)**4;weights/=weights.sum(-1,keepdims=True)
    matrices=[similarity(a,b,c,d) for a,b,c,d in zip(chain[:-1],chain[1:],target[:-1],target[1:])]
    hom=np.column_stack((grid,np.ones(len(grid))))
    mapped=sum(weights[:,i,None]*(hom@matrix.T) for i,matrix in enumerate(matrices))+OFFSET
    # Premultiplied interpolation keeps translucent sprite edges clean.
    tex=texture.astype(np.float32);tex[:,:,:3]*=tex[:,:,3:4]/255
    for row in range(len(ys)-1):
        for col in range(len(xs)-1):
            k=row*len(xs)+col
            for ids in ([k,k+1,k+len(xs)],[k+1,k+len(xs)+1,k+len(xs)]):
                s=np.float32(grid[ids]);q=np.float32(mapped[ids])
                edge1=q[1]-q[0];edge2=q[2]-q[0]
                if abs(edge1[0]*edge2[1]-edge1[1]*edge2[0])<.01:continue
                xmin=max(0,int(np.floor(q[:,0].min())));xmax=min(SIZE,int(np.ceil(q[:,0].max()))+1)
                ymin=max(0,int(np.floor(q[:,1].min())));ymax=min(SIZE,int(np.ceil(q[:,1].max()))+1)
                if xmin>=xmax or ymin>=ymax:continue
                qlocal=q-[xmin,ymin]
                matrix=cv2.getAffineTransform(s,np.float32(qlocal))
                warped=cv2.warpAffine(tex,matrix,(xmax-xmin,ymax-ymin),flags=cv2.INTER_LINEAR)
                mask=np.zeros((ymax-ymin,xmax-xmin),np.uint8);cv2.fillConvexPoly(mask,np.round(qlocal).astype(np.int32),255)
                alpha=warped[:,:,3:4]/255
                warped[:,:,:3]/=np.maximum(alpha,1e-6)
                section=out[ymin:ymax,xmin:xmax];section[mask>0]=warped.clip(0,255).astype(np.uint8)[mask>0]
    return out

forward=np.array([.707,.424]);lateral=np.array([-.707,.424]);root=np.array([194.,429.])
def project(side,f,height):return root+lateral*side+forward*f-[0,height]
def leg(side,phase,bob):
    phase%=1;stance=.6;stride=172;reach=stride*stance/2
    if phase<stance:
        f=reach-stride*phase;lift=0
    else:
        t=(phase-stance)/(1-stance);f=-reach+2*reach*(t*t*(3-2*t));lift=24*math.sin(math.pi*t)
    hiph=153-bob;ankleh=12+lift;delta=np.array([f,ankleh-hiph]);distance=np.linalg.norm(delta)
    length=max(81,distance/2+.01)
    knee=np.array([0,hiph])+delta/2+np.array([-delta[1],delta[0]])/distance*math.sqrt(max(0,length**2-distance**2/4))
    hip=project(side,0,hiph);k=project(side,knee[0],knee[1]);ankle=project(side,f,ankleh)
    toe=ankle+forward*35+[0,8 if phase<stance else 2]
    return np.array([hip,k,ankle,toe]),phase<stance

frames=[];records=[]
for i in range(32):
    p=i/32;bob=4*(1-math.cos(p*math.tau*2));lean=2
    angle=math.sin(p*math.tau)*1.7
    matrix=cv2.getRotationMatrix2D((194.,270.),angle,1);matrix[:,2]+=[lean,bob]
    def torso(q):return np.append(q,1)@matrix.T
    targets={};contacts={}
    targets['rleg'],contacts['r']=leg(17,p,bob)
    targets['lleg'],contacts['l']=leg(-17,p+.5,bob)
    for name,sign in [('rarm',1),('larm',-1)]:
        chain=src[name];shoulder=torso(chain[0]);swing=-math.cos(p*math.tau)*sign*35
        wrist=torso(chain[2])+forward*swing
        elbow=(shoulder+wrist)/2+forward*10
        hand=wrist+(chain[3]-chain[2])
        targets[name]=np.array([shoulder,elbow,wrist,hand])
    result=Image.new('RGBA',(SIZE,SIZE))
    for name in ['lleg','rleg','larm']:result.alpha_composite(Image.fromarray(warp_limb(layers[name],src[name],targets[name])))
    bodymatrix=matrix.copy();bodymatrix[:,2]+=OFFSET
    result.alpha_composite(Image.fromarray(cv2.warpAffine(body,bodymatrix,(SIZE,SIZE),flags=cv2.INTER_LINEAR)))
    result.alpha_composite(Image.fromarray(warp_limb(layers['rarm'],src['rarm'],targets['rarm'])))
    result.save(OUT/f'Walk_SE_{i:02}.png')
    display=Image.new('RGBA',result.size,(27,31,29,255));display.alpha_composite(result)
    dr=ImageDraw.Draw(display);dr.line((20,root[1]+OFFSET[1],540,root[1]+OFFSET[1]),fill=(100,125,110),width=1)
    dr.text((12,12),f'Whole-body rig study / frame {i:02} / NOT APPROVED',fill=(230,210,170))
    frames.append(display.convert('RGB'))
    records.append({'frame':i,'root':(root+OFFSET).tolist(),'rightHand':(targets['rarm'][3]+OFFSET).tolist(),'leftHand':(targets['larm'][3]+OFFSET).tolist(),'contacts':contacts,'visualApproval':False})
frames[0].save(OUT/'Walk_SE.gif',save_all=True,append_images=frames[1:],duration=30,loop=0)
board=Image.new('RGB',(2240,1120),(27,31,29))
for j,i in enumerate(range(0,32,4)):board.paste(frames[i],((j%4)*560,(j//4)*560))
board.save(OUT/'Walk_SE.jpg',quality=94)
(OUT/'Walk_SE.json').write_text(json.dumps(records,indent=2))
print('Wrote 32 full-body study frames; no production selection or import changed.')
