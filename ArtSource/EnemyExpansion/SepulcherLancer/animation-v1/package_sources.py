from pathlib import Path
import json, zipfile, hashlib
root=Path(__file__).resolve().parent
m=json.loads((root/'atlas-manifest.json').read_text())
checks=json.loads((root/'crop-check.json').read_text())
assert len(m['sheets'])==27
assert checks['walkCells']==128 and checks['attackCells']==192
assert all(not e['empty'] and not e['overlappingMaskRows'] for e in checks['checks'])
files=[e['file'] for e in m['sheets']]+['turnaround.png','reference-prompt.txt','generation-prompts.json','correction-prompts.json','nw-recovery-correction.txt','README.md','QA.md','HOME_PC_TASK.md','atlas-manifest.json','crop-check.json','preview.html','preview-template.html','build_review.py','check_manifest.py','check_playback.cjs','package_sources.py']
verification={'status':'Source-art review draft, UE5 untested','sheets':27,'walkingSourcePoses':128,'attackSourcePoses':192,'effectSourcePoses':40,'browserVisualTest':False,'cropChecks':checks,'files':[{ 'file':f,'sha256':hashlib.sha256((root/f).read_bytes()).hexdigest()} for f in files]}
(root/'verification.json').write_text(json.dumps(verification,indent=2))
files.append('verification.json')
target=root.parent.parent/'sepulcher-lancer-animation-v1.zip'
with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as z:
 for f in files:z.write(root/f,'SepulcherLancer/animation-v1/'+f)
print(json.dumps({'zip':str(target),'files':len(files),'bytes':target.stat().st_size}))
