from pathlib import Path
import math,json
import numpy as np
from PIL import Image
from prepare_grounded_hero import foot,project,joints,OUT

checks=0
def check(ok,message):
    global checks
    checks+=1
    assert ok,message

names=json.loads((OUT/'manifest.json').read_text())
check(len(names)==1072,'complete eight-direction set')
for name in names:
    im=Image.open(OUT/f'{name}.png')
    check(im.mode=='RGBA' and im.size==(480,480),name+' format')
    amin,amax=im.getchannel('A').getextrema()
    check(amin==0 and amax>=250,name+' genuine alpha') # Authored attack art peaks at 254.
    box=im.getbbox();check(box is not None and box[0]>0 and box[1]>0 and box[2]<480 and box[3]<480,name+' clipped')
for d in range(8):
    direction=np.array([math.sin(d*math.pi/4),-math.cos(d*math.pi/4)])
    factor=1/math.sqrt(direction[0]**2+(direction[1]/.6)**2)
    for run in (False,True):
        stride=(210 if run else 112)/1.375*factor
        stance=.36 if run else .60
        for t in np.linspace(.001,stance-.002,40):
            a=joints(d,1,t,run,0,5)[2]
            b=joints(d,1,t+.001,run,0,5)[2]+direction*stride*.001
            check(np.linalg.norm(a-b)<1e-8,'planted ankle slides')
        for t in np.linspace(0,.999,100):
            feet=[foot(t,run),foot((t+.5)%1,run)]
            if not run:check(any(f[3] for f in feet),'walk has unintended aerial phase')
        if run:check(not foot(.4,True)[3] and not foot(.9,True)[3],'run needs flight')
print(f'GROUNDED_ASSET_VERIFY {checks} checks, 0 errors')
