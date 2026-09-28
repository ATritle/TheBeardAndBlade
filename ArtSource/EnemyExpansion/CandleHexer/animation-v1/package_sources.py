from pathlib import Path
import json,zipfile
root=Path(__file__).resolve().parent
m=json.loads((root/'atlas-manifest.json').read_text())
checks=json.loads((root/'crop-check.json').read_text())
assert len(m['sheets'])==27 and len(m['entries'])==19
assert sum(len(e['frames']) for e in m['entries'] if e['state']=='walk')==128
assert sum(len(e['frames']) for e in m['entries'] if e['state']=='attack')==192
assert all(e['empty']==0 and e['overlappingMaskRows']==0 for e in checks['checks'])
result={'sourceSheets':27,'characterSourceCells':320,'effectSourceCells':40,'selectedEffectPlaybackCells':38,'emptyCells':0,'overlappingPoseMaskRows':0,'scriptPlaybackCheck':'Passed minimal-DOM check; not visual browser validation','ue5Tested':False}
(root/'verification.json').write_text(json.dumps(result,indent=2))
names=[e['file'] for e in m['sheets']]+['turnaround.png','reference-prompt.txt','generation-prompts.json','correction-prompts.json','README.md','QA.md','HOME_PC_TASK.md','atlas-manifest.json','crop-check.json','verification.json','preview.html','preview-template.html','build_review.py','check_manifest.py','check_playback.cjs','package_sources.py']
names.append('nw-correction-prompts.json')
out=root.parent.parent/'candle-hexer-animation-v1.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
 for name in names:z.write(root/name,'CandleHexer/animation-v1/'+name)
with zipfile.ZipFile(out) as z:assert z.testzip() is None
print(json.dumps({'zip':str(out),'files':len(names),'bytes':out.stat().st_size,'checks':result}))
