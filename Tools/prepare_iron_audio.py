"""Lossy MP3 source retained honestly; PCM conversion is not an original WAV."""
from pathlib import Path
import subprocess,json,sys
root=Path(__file__).resolve().parents[1]
src=root/'ArtSource/Bosses/IronMatriarch/intro-v1/breviceps-dragon-466830-hq.mp3'
out=root/'ArtSource/Bosses/IronMatriarch/runtime-v1'
clips={'IronSlamVoice':(0,.95),'IronBreathVoice':(.55,1.25),'IronMeteorVoice':(1.25,1.6),'IronHurtVoice':(2.5,.65),'IronDeathVoice':(0,3.27)}
report={}
for name,(start,length) in clips.items():
    subprocess.run([sys.argv[1],'-y','-hide_banner','-loglevel','error','-ss',str(start),'-t',str(length),'-i',str(src),'-af',f'volume=0.7,afade=t=in:d=0.012,afade=t=out:st={length-.06}:d=0.06','-ar','44100','-c:a','pcm_s16le',str(out/(name+'.wav'))],check=True)
    report[name]={'source':src.name,'start':start,'duration':length,'artist':'Breviceps','license':'CC0','url':'https://freesound.org/people/Breviceps/sounds/466830/','source_quality':'Supplied high-quality MP3; original WAV not available','pitch':'unchanged'}
(out/'audio-manifest.json').write_text(json.dumps(report,indent=2))
print('Prepared',len(clips),'gameplay vocal clips')
