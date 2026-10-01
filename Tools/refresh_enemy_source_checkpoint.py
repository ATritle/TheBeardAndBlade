"""Reconcile selected source jobs with local files. Does not approve art or touch UE assets."""
import json, re, argparse, subprocess
from pathlib import Path
from datetime import datetime
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'ArtSource/EnemyExpansion'
HANDOFF=BASE/'HomePCHandoff-v2'
def read(path): return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path,value):
    if path.exists() and read(path)==value:return
    path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
parser=argparse.ArgumentParser();parser.add_argument('--refresh',action='store_true');args=parser.parse_args()
old=read(HANDOFF/'inventory.json'); enemies=[];all_jobs=[];saved=[];missing=[];errors=[];validated=0
for enemy in old['enemies']:
    src=BASE/enemy['folder']/'animation-v2'
    jobs=read(src/'generation-prompts.json')['jobs']
    names=[j['file'] for j in jobs]
    if len(names)!=len(set(names)):errors.append(enemy['name']+': duplicate selected jobs')
    available=[];absent=[]
    for job in jobs:
        file=src/job['file'];all_jobs.append(job)
        if file.exists():
            available.append(job);saved.append(job)
            try:
                with Image.open(file) as im:
                    if im.mode!='RGBA':errors.append(str(file.relative_to(ROOT))+': not RGBA')
                    im.verify()
                validated+=1
            except Exception as exc: errors.append(str(file.relative_to(ROOT))+': '+str(exc))
        else:absent.append(job);missing.append(job)
    enemies.append(dict(slug=enemy['slug'],name=enemy['name'],folder=enemy['folder'],savedSheets=len(available),plannedSheets=len(jobs),missing=[j['file'] for j in absent]))
    if args.refresh:
        write(src/'selected-files.json',sorted(j['file'] for j in available))
        write(src/'missing-jobs.json',absent)
        spec=read(src/'enemy-spec.json')
        spec['sourceProgress'].update(selectedSheetsSaved=len(available),selectedSheetsPlanned=len(jobs),visualReviewComplete=False,ueImportTested=False,status='INCOMPLETE_REVIEW_DRAFT')
        write(src/'enemy-spec.json',spec)
        text=(src/'README.md').read_text(encoding='utf-8')
        text=re.sub(r'\d+/\d+ selected sheets saved',f'{len(available)}/{len(jobs)} selected sheets saved',text)
        (src/'README.md').write_text(text,encoding='utf-8')
planned={str(Path(j['root'])/j['file']).replace('\\','/') for j in all_jobs}
for j in all_jobs:
    ref=j.get('reference')
    if ref and not (ROOT/ref).exists() and ref not in planned:errors.append('Unresolvable reference: '+ref)
report=dict(sourceCheckOnly=True,validatedPNGs=validated,selectedSheets=len(saved),remainingSheets=len(missing),plannedSheets=len(all_jobs),errors=errors,ueTested=False)
write(HANDOFF/'local-source-audit.json',report)
if errors:raise SystemExit(json.dumps(report,indent=2))
if args.refresh:
    # Keep the checkpoint's production order; filenames may change after corrections.
    def key(job):return (job['root'],job['state'],job.get('direction'))
    for filename,records in [('all-jobs.json',all_jobs),('saved-jobs.json',saved),('missing-jobs.json',missing)]:
        original=json.loads(subprocess.check_output(['git','show','HEAD:ArtSource/EnemyExpansion/HomePCHandoff-v2/'+filename],cwd=ROOT,text=True))
        order={key(job):index for index,job in enumerate(original)}
        records.sort(key=lambda job:order.get(key(job),len(order)))
    validation=read(HANDOFF/'validation.json')
    validation.update(selectedSheets=len(saved),remainingSheets=len(missing),referencesResolvableOrPlanned=True,renderedBrowserTested=False,ueTested=False)
    write(HANDOFF/'validation.json',validation)
    write(HANDOFF/'inventory.json',dict(createdLocal=datetime.now().isoformat(timespec='seconds'),savedSheets=len(saved),plannedSheets=len(all_jobs),remainingSheets=len(missing),enemies=enemies))
    for filename,records in [('all-jobs.json',all_jobs),('saved-jobs.json',saved),('missing-jobs.json',missing)]:write(HANDOFF/filename,records)
    text=(HANDOFF/'README.md').read_text(encoding='utf-8')
    text=re.sub(r'\d+ of \d+ selected sprite sheets',f'{len(saved)} of {len(all_jobs)} selected sprite sheets',text)
    text=re.sub(r'\d+ sheets remain ungenerated',f'{len(missing)} sheets remain ungenerated',text)
    (HANDOFF/'README.md').write_text(text,encoding='utf-8')
    text=(HANDOFF/'preview.html').read_text(encoding='utf-8')
    text=re.sub(r'\d+/\d+ sheets saved',f'{len(saved)}/{len(all_jobs)} sheets saved',text)
    for e in enemies:
        text=re.sub('(<h2>'+re.escape(e['name'])+r'</h2><p>)\d+/\d+ sheets; \d+ missing',lambda m:m[1]+f"{e['savedSheets']}/{e['plannedSheets']} sheets; {len(e['missing'])} missing",text)
    (HANDOFF/'preview.html').write_text(text,encoding='utf-8')
print(json.dumps(report))
