"""Measured single-pose corrections assembled as a separate review sequence."""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Intermediate/HeroAuthoring/deps'))
import cv2,numpy as np
from PIL import Image
ART=ROOT/'ArtSource/HeroFullBodyV1'
paths=['production/Walk_SE_right_contact_v3.png','crops/Walk_3/01.png',
       'crops/Walk_3/02.png','production/Walk_SE_right_push_v1.png',
       'crops/Walk_3/04.png','production/Walk_SE_left_loading_v1.png',
       'production/Walk_SE_right_swing_v1.png','production/Walk_SE_left_push_v1.png']
out=ART/'crops/Walk_SE_corrected';out.mkdir(exist_ok=True)
records=[]
for i,path in enumerate(paths):
    im=Image.open(ART/path).convert('RGBA');a=np.array(im)
    n,labels,stats,centers=cv2.connectedComponentsWithStats((a[:,:,3]>40).astype(np.uint8),8)
    largest=1+np.argmax(stats[1:,cv2.CC_STAT_AREA]);mask=(labels==largest).astype(np.uint8)
    keep=cv2.dilate(mask,np.ones((5,5),np.uint8))>0
    a[~keep]=0;a[a[:,:,3]<32]=0
    cleaned=Image.fromarray(a);bounds=cleaned.getbbox();cleaned.crop(bounds).save(out/f'{i:02}.png')
    records.append({'index':i,'source':path,'measuredRect':list(bounds),'visualApproval':False})
(out/'source-records.json').write_text(json.dumps(records,indent=2))
print('Extracted eight corrected walking candidates; production selection unchanged.')
