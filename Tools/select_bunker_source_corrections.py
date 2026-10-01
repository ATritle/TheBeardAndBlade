"""Register visually reviewed source candidates; no gameplay/runtime changes."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'ArtSource/EnemyExpansion'
for enemy in ['RivetGunner','BreachHound','TrenchShivver']:
    path=root/enemy/'animation-v2/generation-prompts.json'
    data=json.loads(path.read_text())
    if enemy in ['RivetGunner','TrenchShivver']:
        for direction in (['E','NE'] if enemy=='RivetGunner' else ['E','SW']):
            meta=json.loads((root/enemy/f'continuation-v1/recovery-{direction}-prompt.json').read_text())
            job=next(j for j in data['jobs'] if j['file']==meta['target'])
            job['prompt']=meta['prompt'];job['generationMode']='built-in imagegen'
    else:
        meta=json.loads((root/enemy/'continuation-v1/idle-E-margin-v4-prompt.json').read_text())
        job=next(j for j in data['jobs'] if j['state']=='idle' and j['direction']=='E')
        for field in ['file','prompt','reference','rows','columns','frames']:job[field]=meta[field]
        job['generationMode']='built-in imagegen'
    path.write_text(json.dumps(data,indent=2)+'\n')
    print(enemy,'source selections updated; UE validation pending')
