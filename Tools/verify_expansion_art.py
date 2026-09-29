"""Mechanical asset checks; visual anatomy/grips still require the UE review."""
import json
from collections import defaultdict
import numpy as np
from PIL import Image
from prepare_expansion import ROOT,NAMES,DIRS,body_height

total=0
for name in NAMES:
    folder=ROOT/'ArtSource/EnemyExpansion'/name
    records=json.loads((folder/'runtime-v1/runtime-manifest.json').read_text())
    originals=json.loads((folder/'animation-v1/atlas-manifest.json').read_text())['entries']
    counts=defaultdict(int);scales=defaultdict(set);heights=[]
    for r in records:
        im=Image.open(ROOT/'Content/Art/EnemyExpansion'/name/(r['name']+'.png')).convert('RGBA')
        a=np.asarray(im.getchannel('A'));bb=im.getbbox()
        assert bb and a.max()>=32,r['name']
        assert not a[0].any() and not a[-1].any() and not a[:,0].any() and not a[:,-1].any(),r['name']
        assert im.size==((256,256) if '_FX_' in r['name'] else (512,512)),r['name']
        if '_FX_' not in r['name']:
            direction,state,index=r['name'][len(name)+1:].split('_')
            counts[direction,state]+=1
            assert r['root']==[256,450]
            assert abs(bb[3]-451)<=2,(r['name'],bb)
            scales[direction,state,r['source']].add(r['scale'])
            if state=='idle':heights.append(round(body_height(im,{'pivotX':256})*290/512,1))
            if state in ('walk','attack'):
                entry=next(e for e in originals if e['direction']==direction and e['state']==state)
                assert r['crop']==entry['frames'][r['sourceFrame']],r['name']
        total+=1
    for d in DIRS:
        for state,count in [('idle',4),('walk',8),('attack',16),('hurt',4),('death',8)]:
            assert counts[d,state]==count,(name,d,state,counts[d,state])
    assert all(len(s)==1 for s in scales.values()),'per-frame resize causes pumping'
    print(name,len(records),'assets; neutral body gameplay pixels',min(heights),max(heights))
print('EXPANSION_ART_VERIFY',total,'assets PASS')
