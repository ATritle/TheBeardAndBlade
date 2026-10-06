"""Offline measurement assistance, never automatic visual/anchor approval.

Run in the isolated authoring environment. Coordinates are local to each measured
crop. Raw ML estimates are retained for audit and must be visually checked.
"""
from pathlib import Path
import sys, json, argparse
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Intermediate/HeroAuthoring/deps'))
import numpy as np
import mediapipe as mp
from PIL import Image,ImageDraw

ART=ROOT/'ArtSource/HeroFullBodyV1'
OUT=ART/'landmarks';OUT.mkdir(exist_ok=True)
p=argparse.ArgumentParser();p.add_argument('--name',required=True);args=p.parse_args()
KEYS={0:'nose',7:'leftEar',8:'rightEar',11:'leftShoulder',12:'rightShoulder',
13:'leftElbow',14:'rightElbow',15:'leftWrist',16:'rightWrist',
17:'leftPinky',18:'rightPinky',19:'leftIndex',20:'rightIndex',
23:'leftHip',24:'rightHip',25:'leftKnee',26:'rightKnee',
27:'leftAnkle',28:'rightAnkle',29:'leftHeel',30:'rightHeel',31:'leftToe',32:'rightToe'}
EDGES=[(11,12),(11,23),(12,24),(23,24),(11,13),(13,15),(12,14),(14,16),
       (23,25),(25,27),(27,31),(24,26),(26,28),(28,32)]
options=mp.tasks.vision.PoseLandmarkerOptions(
    base_options=mp.tasks.BaseOptions(model_asset_path=str(ROOT/'Intermediate/HeroAuthoring/pose_landmarker_heavy.task')),
    running_mode=mp.tasks.vision.RunningMode.IMAGE,
    min_pose_detection_confidence=.25,min_pose_presence_confidence=.25)
with mp.tasks.vision.PoseLandmarker.create_from_options(options) as model:
    folders=sorted((ART/'crops').glob(args.name))
    for folder in folders:
        if not folder.is_dir():continue
        records=[];board=Image.new('RGB',(1600,1200),(26,28,27))
        for i in range(8):
            im=Image.open(folder/f'{i:02}.png').convert('RGBA')
            pad=64;canvas=Image.new('RGBA',(im.width+2*pad,im.height+2*pad),(95,100,97,255))
            canvas.alpha_composite(im,(pad,pad));rgb=canvas.convert('RGB')
            result=model.detect(mp.Image(image_format=mp.ImageFormat.SRGB,data=np.asarray(rgb)))
            record={'index':i,'sourceSize':list(im.size),'visualApproval':False,'landmarks':{}}
            draw=ImageDraw.Draw(rgb)
            if result.pose_landmarks:
                pts=result.pose_landmarks[0]
                for index,name in KEYS.items():
                    q=pts[index];record['landmarks'][name]={'xy':[round(q.x*rgb.width-pad,2),round(q.y*rgb.height-pad,2)],'visibility':round(q.visibility,4),'presence':round(q.presence,4)}
                for a,b in EDGES:
                    c=(255,110,85) if a%2==0 else (80,185,255)
                    pa,pb=pts[a],pts[b];draw.line((pa.x*rgb.width,pa.y*rgb.height,pb.x*rgb.width,pb.y*rgb.height),fill=c,width=3)
                for index,name in KEYS.items():
                    q=pts[index];x,y=q.x*rgb.width,q.y*rgb.height
                    draw.ellipse((x-3,y-3,x+3,y+3),fill=(255,240,80));draw.text((x+5,y),str(index),fill=(255,255,255))
            else:record['error']='No pose detected; manual landmarks required'
            records.append(record)
            rgb.thumbnail((390,555),Image.Resampling.LANCZOS)
            x=i%4*400;y=i//4*600;board.paste(rgb,(x+(400-rgb.width)//2,y))
            ImageDraw.Draw(board).text((x+8,y+570),f'{folder.name} / {i} - UNREVIEWED',fill=(240,210,145))
        (OUT/f'{folder.name}.json').write_text(json.dumps({'name':folder.name,'method':'MediaPipe Heavy 1.0.1 offline estimate','approved':False,'frames':records},indent=2))
        board.save(OUT/f'{folder.name}.jpg',quality=94)
        print(folder.name,sum(bool(f['landmarks']) for f in records),flush=True)
