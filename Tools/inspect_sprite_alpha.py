"""Composite source alpha for honest visual QA; never changes source art."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('output');a=p.parse_args()
im=Image.open(a.source).convert('RGBA')
bg=Image.new('RGBA',im.size,(48,58,68,255));draw=ImageDraw.Draw(bg)
for y in range(0,im.height,24):
    for x in range(0,im.width,24):
        if (x//24+y//24)%2:draw.rectangle((x,y,x+23,y+23),fill=(70,80,90,255))
bg.alpha_composite(im);bg.convert('RGB').save(a.output)
