"""Prepare one attack yell and three hurt cues; original pitch is preserved."""
from pathlib import Path
import json,sys
import numpy as np
import soundfile as sf

root=Path(__file__).resolve().parents[1]
source=root/'AudioSource/Foley/Rime'
clips={
    'RimeAttack':[(.28,1.43)], # Exclude spoken "ALRIGHT" and "YES" clips.
    'RimePain':[(.32,1.17),(4.89,6.14),(9.29,10.33)],
}
sources={
    'RimeAttack':('weirdwarriornosieslol.mp3','SkyRae','https://opengameart.org/content/female-warrior-cheer'),
    'RimePain':('female_hurt_grunts_groans_1.ogg','AuraVoice / Nocturnal_Vanguard','https://opengameart.org/content/female-hurt-grunts-groans'),
}
report=json.loads((source/'manifest.json').read_text()) if (source/'manifest.json').exists() else {}
for retired in ('FoleyRimeAttack1','FoleyRimeAttack2'):
    report.pop(retired,None)
for family,ranges in clips.items():
    if '--attacks-only' in sys.argv and family!='RimeAttack':continue
    filename,artist,url=sources[family]
    samples,rate=sf.read(source/filename,always_2d=True)
    samples=samples.mean(axis=1)
    for i,(start,end) in enumerate(ranges):
        name=f'Foley{family}{i}'
        clip=samples[round(start*rate):round(end*rate)].copy()
        clip-=clip.mean()
        fade=round(rate*.008)
        clip[:fade]*=np.linspace(0,1,fade);clip[-fade:]*=np.linspace(1,0,fade)
        clip*=.48/max(abs(clip).max(),1e-9)
        assert np.isfinite(clip).all() and abs(clip).max()<1
        sf.write(source/f'{name}.wav',clip,rate,subtype='PCM_16')
        report[name]={'source':filename,'start':start,'end':end,'seconds':len(clip)/rate,'peak':float(abs(clip).max()),'license':'CC0','artist':artist,'url':url}
(source/'manifest.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
