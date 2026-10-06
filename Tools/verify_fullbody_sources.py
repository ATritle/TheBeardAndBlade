"""Coverage is separate from animation approval. Strict mode refuses unreviewed art."""
from pathlib import Path
import argparse,json,hashlib
from PIL import Image
root=Path(__file__).resolve().parents[1]/'ArtSource/HeroFullBodyV1'
p=argparse.ArgumentParser();p.add_argument('--sources-only',action='store_true');args=p.parse_args()
spec=json.loads((root/'production-spec.json').read_text())
if (root/'additional-spec.json').exists():spec['clips']+=json.loads((root/'additional-spec.json').read_text())['clips']
selections=json.loads((root/'selections.json').read_text())
errors=[];not_ready=[];manifest=[];hashes={}
for clip in spec['clips']:
    for direction in range(8):
        name=f"{clip['name']}_{direction}"
        relative=selections.get(name,f'production/{name}.png');path=root/relative
        if not path.exists():errors.append(f'{name}: missing source');continue
        im=Image.open(path)
        if im.mode!='RGBA':errors.append(f'{name}: missing RGBA transparency');continue
        lo,hi=im.getchannel('A').getextrema()
        if lo!=0 or hi<240:errors.append(f'{name}: invalid alpha extrema {lo},{hi}')
        sha=hashlib.sha256(path.read_bytes()).hexdigest()
        if sha in hashes:errors.append(f'{name}: source duplicates {hashes[sha]}')
        hashes[sha]=name
        regpath=root/'crops'/name/'registration.json'
        reg=json.loads(regpath.read_text()) if regpath.exists() else {}
        frames=reg.get('frames',[])
        if len(frames)!=8 or any('error' in f for f in frames):errors.append(f'{name}: eight measured crops not available')
        ready=len(frames)==8 and all(f.get('visualApproval') and f.get('root') is not None and not f.get('flags') for f in frames)
        if clip['name'] in ['Idle','Walk','Run','MeleeSlash','MeleeBackhand','MeleeCombo','Block']:
            ready=ready and all(f.get('rightHand') is not None and f.get('weaponAngle') is not None for f in frames)
        if clip['name']=='BowDrawFire':
            ready=ready and len(frames)==8 and all(frames[i].get('bowMuzzle') is not None for i in [2,3,4])
        ready=ready and reg.get('engineVerified',False)
        if not ready:not_ready.append(name)
        manifest.append({'name':name,'clip':clip['name'],'direction':direction,
                         'source':relative,'sha256':sha,'nativeSize':list(im.size),
                         'measuredFrames':sum('error' not in f for f in frames),'readyForImport':ready})
report={'coverageScope':'Initial 19-action source-art plan; not all seamless gameplay transitions',
        'requiredSheets':len(spec['clips'])*8,'presentSheets':len(manifest),
        'measuredPoses':sum(m['measuredFrames'] for m in manifest),
        'sourceErrors':errors,'unapprovedSheets':not_ready,
        'readyForGame':not errors and not not_ready,'mode':'built-in image_gen',
        'manifest':manifest}
(root/'source-manifest.json').write_text(json.dumps(report,indent=2))
print(json.dumps({k:report[k] for k in ['requiredSheets','presentSheets','measuredPoses','sourceErrors','readyForGame']}))
print('Unapproved sheets:',len(not_ready))
raise SystemExit(1 if errors or (not args.sources_only and not_ready) else 0)
