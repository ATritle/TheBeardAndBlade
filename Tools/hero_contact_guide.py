"""Measured rough pose edit for art correction; this is NOT a shipping frame."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Intermediate/HeroAuthoring/deps'))
import cv2,numpy as np
from PIL import Image,ImageDraw
art=ROOT/'ArtSource/HeroFullBodyV1'
im=Image.open(art/'crops/Walk_3/00.png').convert('RGBA')
a=np.array(im);h,w=a.shape[:2]
polys=[[(171,262),(199,284),(174,328),(142,365),(132,387),(143,405),(142,421),(108,425),(89,412),(79,389),(84,365),(111,321),(129,296)],
       [(198,262),(225,269),(230,307),(249,353),(267,393),(290,414),(328,422),(337,439),(322,454),(270,456),(248,445),(236,418),(220,380),(202,343),(181,303)]]
layers=[]
for poly in polys:
    mask=Image.new('L',im.size);ImageDraw.Draw(mask).polygon(poly,fill=255)
    layer=a.copy();layer[:,:,3]=np.minimum(layer[:,:,3],np.asarray(mask));layers.append(layer)
    a[np.asarray(mask)>0,3]=0
# The rough pose exchanges forward/back reach without reflecting the character.
# Anatomy, missing overlap and seams are then explicitly repaired with imagegen.
src=[[[172,276],[126,348],[121,399]],[[207,280],[230,355],[280,428]]]
dst=[[[172,276],[207,338],[261,416]],[[207,280],[157,329],[126,377]]]
yy=np.arange(h)[:,None]
green=(a[:,:,1]>a[:,:,0]*1.1)&(a[:,:,1]>a[:,:,2]*1.15)
a[(yy>280)&~green,3]=0
canvas=Image.new('RGBA',(480,560));base=Image.fromarray(a)
for i in [1,0]:
    s0,s1=np.array(src[i][0],float),np.array(src[i][2],float)
    t0,t1=np.array(dst[i][0],float),np.array(dst[i][2],float)
    sv=s1-s0;tv=t1-t0
    sn=np.array([-sv[1],sv[0]])/np.linalg.norm(sv)*30
    tn=np.array([-tv[1],tv[0]])/np.linalg.norm(tv)*30
    matrix=cv2.getAffineTransform(np.float32([s0,s1,s0+sn]),np.float32([t0,t1,t0+tn]))
    warped=cv2.warpAffine(layers[i],matrix,(400,500),flags=cv2.INTER_CUBIC)
    canvas.alpha_composite(Image.fromarray(warped),(40,30))
canvas.alpha_composite(base,(40,30))
out=art/'correction-guides';out.mkdir(exist_ok=True)
canvas.save(out/'SE-right-contact-rough.png')
