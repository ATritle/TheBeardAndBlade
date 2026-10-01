"""Create a lossless measured crop for manual visual inspection only."""
import argparse
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('output');p.add_argument('--box',required=True);p.add_argument('--scale',type=int,default=1);a=p.parse_args()
im=Image.open(a.source).convert('RGBA');print(im.size)
im=im.crop(tuple(map(int,a.box.split(','))));im=im.resize((im.width*a.scale,im.height*a.scale),Image.Resampling.NEAREST);im.save(a.output)
